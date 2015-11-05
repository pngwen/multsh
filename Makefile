CC=gcc
CFLAGS=-g

all:multsh unitTests
multsh:multsh.o tty_functions.o
unitTests:unitTests.o buffer.o

clean:
	rm -f *.o a.out multsh
