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
#ifndef BUFFER_H
#define BUFFER_H

#define BUFSIZE 2048
struct buffer {
  char buf[BUFSIZE];
  int start;
  int size;
};


/* the remaining capacity of the buffer (b is a buffer) */
#define BUF_REMAINING(b) (BUFSIZE - (b).size)

/* initialize the buffer to default conditions */
#define BUF_INIT(b) {b.start = 0; b.size = 0;}

/* copy the contents of a void buffer into the current buffer
     b - the circular buffer structure
     ar - the to copy from
     n - the size of the character data
   This function returns the total number of bytes copied into the buffer. 
   If ar and b have overlapping data elements, you're gonna have a bad time!
*/
int buf_append(struct buffer *b, const void *ar, int n);

/* updates the start and size for the consumption of n bytes.  
   This function returns the total number of bytes consumed. 
     dest - The destination of the consumed data.  If it is non-null,
            consumed bytes are copied here.  Otherwise, we just do the math.
     b    - The buffer to consume from
     n    - The number of bytes to attempt to consume
   Allowing dest and b to have overlapping data would be ill-advised.
*/
int buf_consume(void *dest, struct buffer *b, int n);
#endif
