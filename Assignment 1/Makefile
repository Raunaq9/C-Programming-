CC=gcc
CFLAGS=-Wall -pedantic -std=gnu99 -g
HLINKS=-I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a1
.PHONY:= clean

all: wordleHelper

wordleHelper: wordleHelper.c
	$(CC) $(CFLAGS) wordleHelper.c $(HLINKS) -o wordle-helper

clean:
	rm -f *.o
	rm -f wordleHelper
