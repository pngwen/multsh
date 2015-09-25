/* This is a little program for sharing a pty with anyone who joins in.
 * It is unecrypted, unsecured, and wicked cool.  
 * Enjoy!  and please use responsibly.
 *
 * Installation: just compile this file and put it wherever you wish.
 * 
 *    Copyright (C) Robert Lowe <robert.lowe@maryvillecollege.edu>
 *                                                                        
 * This program is free software. You may use, modify, and redistribute it
 * under the terms of the GNU General Public License as published by the  
 * Free Software Foundation, either version 3 or (at your option) any      
 * later version. This program is distributed without any warranty.  See  
 * the file COPYING.gpl-v3 for details.    
 */
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 2048

struct termSession {
  int fd;       /* slave pty for session */
  int sfd;      /* socket file descriptor */
  int pid;      /* pid of the child process */
  int ridx;     /* read index buffer */
  int wridx;    /* write index for buff */
  int rsize;    /* size of the read buffer */
  int wrsize;   /* size of the write buffer */
  unsigned char rbuf[BUFSIZE];
  unsigned char wrbuf[BUFSIZE];
};

// Global vars
static const char *shell;
static int port;

int getPty();
int openSlave(int fd, int flags);
void parseCmdLine(int argc, char **argv);
int startShell();


int main(int argc, char **argv)
{
  parseCmdLine(argc, argv);
  printf("%s\t%d\n", shell, port);
	
  return 0;
}


//Return a master pty fd -1 on failure
int getPty()
{
  int fd;
	
  //open a pty master
  fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  return fd;
}


//open the slave pty with guven flags
int openSlave(int fd, int flags) 
{
  char *sname;
	
  //take ownership of the slave pty
  if(grantpt(fd) == -1) {
    return -1;
  }
	
  //unlock the slave
  if(unlockpt(fd) == -1) {
    return -1;
  }
	
  //get the slave name
  sname = ptsname(fd);
  if(!sname) {
    return -1;
  }
	
  //get and return the fd
  return open(sname, flags);
}


void parseCmdLine(int argc, char **argv)
{
  int opt;
	
  //set defaults
  shell = getenv("SHELL");
  port = 1337;
	
  while((opt=getopt(argc, argv, "p:l:")) != -1) {
    switch(opt) {
    case 'p':
      port = atoi(optarg);
      break;
		  
    case 'l':
      shell = optarg;
      break;
		  
    case 'h':
    case '?':
    default:
      fprintf(stderr, "\nUsage: %s\n", argv[0]);
      fprintf(stderr, "  -h\n");
      fprintf(stderr, "  -p port\n");
      fprintf(stderr, "  -l login shell\n\n");
      exit(-1);
      break;
    }
  }
}

//Spawn a login shell. 
//Returns the fd of the pts master of the
//shell
int startShell() 
{
  int pid;
  int fd;
  int sfd;
	
  //get the master fd
  fd = getPty();
  if(fd == -1) return -1;
	
  //open the slave
  sfd = openSlave(fd, O_RDWR | O_NOCTTY);
  if(sfd == -1) {
    close(fd);
    return -1;
  }
	
  // spawn
  pid = fork();
  if(pid == -1) {
    close(fd);
    close(sfd);
    return -1;
  }
	
  //return fd to parent
  if(pid) {
    return fd;
  }
	
  //set up the child fd's
  close(fd);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  dup(sfd);
  dup(sfd);
  dup(sfd);
	
  //start the shell
  execlp(shell, shell, NULL);
}
	
