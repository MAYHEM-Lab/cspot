all: server server-1

server: mio/mio.o mio/mio.h mio/mymalloc.o lsema.o lsema.h server.c miotest.h
	gcc -g -Imio -o server server.c mio/mio.o mio/mymalloc.o lsema.o -lpthread

server-1: mio/mio.o mio/mio.h mio/mymalloc.o lsema.o lsema.h server-1.c miotest.h
	gcc -g -Imio -o server-1 server-1.c mio/mio.o mio/mymalloc.o lsema.o -lpthread

sema.o: sema.c sema.h
	gcc -c sema.c

fsema.o: fsema.c fsema.h
	gcc -c -g fsema.c

lsema.o: lsema.c lsema.h
	gcc -c -g lsema.c

mio/mio.o: mio/mio.c mio/mio.h
	cd mio; make
