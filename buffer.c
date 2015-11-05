/* This is a small circular buffer implementation.  This file is part of
 * the multsh program, but may prove useful in other contexts as well.
 *
 *    Copyright (C) Robert Lowe <robert.lowe@maryvillecollege.edu>
 *                                                                        
 * This program is free software. You may use, modify, and redistribute it
 * under the terms of the GNU General Public License as published by the  
 * Free Software Foundation, either version 3 or (at your option) any      
 * later version. This program is distributed without any warranty.  See  
 * the file COPYING.gpl-v3 for details.    
 */
#include <string.h>
#include "buffer.h"

/* copy the contents of a void buffer into the current buffer
     b - the circular buffer structure
     ar - the to copy from
     n - the size of the character data
   This function returns the total number of bytes copied into the buffer. 
   If ar and b have overlapping data elements, you're gonna have a bad time!
*/
int
buf_append(struct buffer *b, const void *ar, int n) {
  int ncopied;
  int cpy;
  int toEnd;
  int start;
  
  //figure out the number of bytes to copy
  if(n <= BUF_REMAINING(*b)) {
    ncopied = n;
  } else {
    ncopied = BUF_REMAINING(*b);
  }

  //work out how much to copy this go
  start = b->start + b->size;
  toEnd = BUFSIZE - start;
  if(toEnd<0) toEnd=0;
  cpy = ncopied < toEnd ? ncopied : toEnd;

  //copy the first chunk
  if(cpy) {
    memcpy(b->buf + start, ar, cpy);
  }

  //copy the remaining chunk (if needs be)
  if(cpy < ncopied) {
    memcpy(b->buf, ar+cpy, ncopied - cpy);
  }

  //update the size
  b->size += ncopied;

  return ncopied;
}


/* updates the start and size for the consumption of n bytes.  
   This function returns the total number of bytes consumed. 
     dest - The destination of the consumed data.  If it is non-null,
            consumed bytes are copied here.  Otherwise, we just do the math.
     b    - The buffer to consume from
     n    - The number of bytes to attempt to consume
   Allowing dest and b to have overlapping data would be ill-advised.
*/
int
buf_consume(void *dest, struct buffer *b, int n) {
  int consumed;
  int toCopy;
  int soFar;
  
  //work out how much to consume
  if(n>b->size) {
    consumed = b->size;
  } else {
    consumed = n;
  }

  //compute the first chunk
  toCopy = BUFSIZE - b->start;
  toCopy = consumed < toCopy ? consumed : toCopy;

  //copy the first chunk
  if(dest) {
    memcpy(dest, b->buf + b->start, toCopy);
  }

  //compute the second chunk
  soFar=toCopy;
  toCopy = consumed - toCopy;
  
  //copy the second chunk
  if(toCopy && dest) {
    memcpy(dest+soFar, b->buf, toCopy);
  }

  //update the sizes and return
  b->size -= consumed;
  b->start += consumed;
  b->start %= BUFSIZE;

  return consumed;
}
