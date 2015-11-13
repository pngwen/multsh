#include <stdio.h>
#include <string.h>
#include "buffer.h"

//test prototypes
void bufferTest();

int
main() {
  bufferTest();

  return 0;
}


void
bufferTest()
{
  struct buffer b;
  int n;
  char s[3];
  int count;

  printf("Buffer Tests\nFilling buffer: ");
  BUF_INIT(b);
  while(buf_append(&b, "aabb", 4) == 4);  //fill the buffer with aabb
  if(b.size == BUFSIZE) {
    printf("[passed]\n");
  } else {
    printf("[failed!]\n");
  }

  //consume an a
  n = buf_consume(s, &b, 1);
  printf("Consuming 1 character: ");
  if(n==1 && s[0]=='a' && b.size == BUFSIZE-1) 
    printf("[passed]\n");
  else
    printf("[failed!]\n");

  //put a c on the end
  printf("Putting a c on the end: ");
  n = buf_append(&b, "c", 1);
  if(n==1 && b.size==BUFSIZE) {
    printf("[passed]\n");
  } else {
    printf("[failed!]\n");
  }

  //consumption test
  count=0;
  printf("Consuming Buffer: ");
  while((n=buf_consume(s, &b, 2))) {
    s[n] = '\0';
    count++;
    if(count==1024 && strcmp(s, "bc")) {
      goto failed;
    } else if(count < 1024 &&count%2 && strcmp(s, "ab")) {
      goto failed;
    } else if(count <1024&&count%2 ==0 && strcmp(s,"ba")) {
      goto failed;
    }
  }

  printf("[passed]\n");
  return;

  failed: printf("[failed!]\n");
}
