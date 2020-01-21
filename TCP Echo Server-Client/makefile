# Makefile for echo server and echo client

echoboth: echoclnt.o echoserv.o
	gcc echoclnt.o -o echoclnt
	gcc echoserv.o -o echoserv

echoserv.o:
	gcc -c echoserv.c

echoclnt.o:
	gcc -c echoclnt.c

clean:
	rm -f echoserv echoclnt *.o

