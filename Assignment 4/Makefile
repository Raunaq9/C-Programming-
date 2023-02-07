# Makefile for psclient and psserver program
CC=gcc
CFLAGS=-Wall -pedantic -pthread -std=gnu99 -g
HLINKS= -I. -L.
LIBS=-lstringmap
.PHONY:= clean

all:stringmap.o libstringmap.so psclient psserver

# Generate executables by linking object files
# Turn stringmap.c into stringmap.o
stringmap.o: stringmap.c
	$(CC) $(CFLAGS) -c stringmap.c $(HLINKS) -o stringmap.o

#Turn stringmap.o into shared library libstringmap.so
libstringmap.so: stringmap.o
	$(CC) -shared $(HLINKS) -o $@ stringmap.o

psclient: psclient.c 
	$(CC) $(CFLAGS) psclient.c  $(HLINKS) -o psclient

psserver: psserver.c libstringmap.so
	$(CC) $(CFLAGS) psserver.c $(HLINKS) $(LIBS) -o psserver

clean:
	rm -f *.o
	rm -f *.so
	rm -f psclient
	rm -f psserver
