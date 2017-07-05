all: server server-1

server: mio/mio.o mio/mio.h mio/mymalloc.o fsema.o fsema.h server.c miotest.h
	gcc -g -Imio -o server server.c mio/mio.o mio/mymalloc.o fsema.o

server-1: mio/mio.o mio/mio.h mio/mymalloc.o fsema.o fsema.h server-1.c miotest.h
	gcc -g -Imio -o server-1 server-1.c mio/mio.o mio/mymalloc.o fsema.o

sema.o: sema.c sema.h
	gcc -c sema.c

fsema.o: fsema.c fsema.h
	gcc -c -g fsema.c

mio/mio.o: mio/mio.c mio/mio.h
	cd mio; make
