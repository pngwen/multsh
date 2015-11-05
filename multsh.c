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
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "tty_functions.h"

#define BUFSIZE 2048

struct session {
  int fd;       /* master pty for session */
  int sfd;      /* socket file descriptor */
  int ridx;     /* read index buffer */
  int wridx;    /* write index for buff */
  int rsize;    /* size of the read buffer */
  int wrsize;   /* size of the write buffer */
  unsigned char rbuf[BUFSIZE];  /* read buffer */
  unsigned char wrbuf[BUFSIZE]; /* write buffer */
  enum {CONTROLLER, CLIENT, SHELL} type;  /* the type of the session */
  struct session *next;
};

/* Global vars */
static const char *shellCmd;
static int port;
static struct session *sessions;  /* linked list of sessions */
static struct session *shell;     /* the shell session (in the session list) */
static int shellPid;
static struct termios termPrev;   /* the starting status of our terminal */
static int run;

int getPty();
int openSlave(int fd, int flags);
void parseCmdLine(int argc, char **argv);
int startShell();
struct session *newSession();
void pollSessions(fd_set *rdSet, fd_set *wrSet);
void processInput(fd_set *rdSet);
void processOutput(fd_set *wrSet);
void fillBuff(int fd, char *buf, int *start, int *size);


int main(int argc, char **argv)
{
  fd_set rdSet, wrSet;
  
  parseCmdLine(argc, argv);

  //start with the controlling session
  sessions=newSession();
  sessions->type = CONTROLLER;

  //start the shell
  startShell();

  //the I/O loop
  run = 1;
  while(run) {
    pollSessions(&rdSet, &wrSet);
    processInput(&rdSet);
    processOutput(&wrSet);
  }
  
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
  shellCmd = getenv("SHELL");
  port = 1337;
	
  while((opt=getopt(argc, argv, "p:l:")) != -1) {
    switch(opt) {
    case 'p':
      port = atoi(optarg);
      break;
		  
    case 'l':
      shellCmd = optarg;
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
//The shell is added to the global environment
//Returns 1 on success, 0 on failure
int startShell() 
{
  int pid;
  int fd;
  int sfd;
  struct session *session;
  
  //get the master fd
  fd = getPty();
  if(fd == -1) return 0;
	
  //open the slave
  sfd = openSlave(fd, O_RDWR | O_NOCTTY);
  if(sfd == -1) {
    close(fd);
    return 0;
  }
	
  // spawn
  pid = fork();
  if(pid == -1) {
    close(fd);
    close(sfd);
    return 0;
  }
	
  //create the session and report success
  if(pid) {
    //create the session and set everything up
    session = newSession();
    if(!session) return 0;
    session->fd=fd;
    session->sfd=-1;
    session->type=SHELL;
    shell = session;
    shellPid = pid;
    return 1;
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
  execlp(shellCmd, shellCmd, NULL);
}
	

/* Creates a new session and links it into the global session list */
struct session *newSession()
{
  struct session *result;

  //create the session and link it in
  result = malloc(sizeof(struct session));
  if(!result) return NULL;
  result->next = sessions;
  sessions = result;

  //the buffers start out empty
  result->ridx=0;
  result->wridx=0;
  result->rsize=0;
  result->wrsize=0;

  //by default we have no fds
  result->fd = -1;
  result->sfd = -1;
  return result;
}


void pollSessions(fd_set *rdSet, fd_set *wrSet)
{
  struct session *cur;
  int maxfd = -1;
  FD_ZERO(rdSet);
  FD_ZERO(wrSet);

  //go through all the sessions and add them to the rd and wr sets
  for(cur = sessions; cur; cur=cur->next) {
    //command uses the stdin and stdout fds
    if(cur->type == COMMAND) {
      FD_SET(STDIN_FILENO, rdSet);
      FD_SET(STDOUT_FILENO, wrSet);
      if(STDIN_FILENO > maxfd) {
	maxfd = STDIN_FILENO;
      }
      if(STDOUT_FILENO > maxfd) {
	maxfd = STDOUT_FILENO;
      }
    } else {
      //everything else uses fd and sfd
      if(cur->fd >= 0) {
	FD_SET(cur->fd, rdSet);
	FD_SET(cur->fd, wrSet);
	if(cur->fd > maxfd)
	  maxfd = cur->fd;
      }
      if(cur->sfd >= 0) {
	FD_SET(cur->sfd, rdSet);
	FD_SET(cur->sfd, wrSet);
	if(cur->sfd > maxfd)
	  maxfd = cur->sfd;
      }
    }
  }

  //now we do the select!
  select(maxfd+1, rdSet, wrSet, NULL, NULL);

  //TODO handle select errors
}


void processInput(fd_set *rdSet)
{
}


void processOutput(fd_set *wrSet)
{
}
