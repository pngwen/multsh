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
#include "buffer.h"

#define BUFSIZE 2048

struct session {
  int fdin;    /* filedescriptor for writing */
  int fdout;   /* filedescriptor for reading */
  struct buffer buf;  /* the buffer for this node */
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

void parseCmdLine(int argc, char **argv);
int startShell();
struct session *newSession();
void pollSessions(fd_set *rdSet, fd_set *wrSet);
void processInput(fd_set *rdSet);
void processOutput(fd_set *wrSet);


int
main(int argc, char **argv)
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




void
parseCmdLine(int argc, char **argv)
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
int
startShell() 
{
  //start the shell
  execlp(shellCmd, shellCmd, NULL);
}
	

/* Creates a new session and links it into the global session list */
struct session *
newSession()
{
  struct session *result;

  //create the session and link it in
  result = malloc(sizeof(struct session));
  if(!result) return NULL;
  result->next = sessions;
  sessions = result;

  //the buffers start out empty
  BUF_INIT(result->buf);

  //by default we have no fds
  result->fdin = -1;
  result->fdout = -1;
  return result;
}


void
pollSessions(fd_set *rdSet, fd_set *wrSet)
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


void
processInput(fd_set *rdSet)
{
}


void
processOutput(fd_set *wrSet)
{
}
