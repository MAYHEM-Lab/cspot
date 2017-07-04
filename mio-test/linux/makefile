all: client

client: mio/mio.o mio/mio.h mio/mymalloc.o fsema.o fsema.h client.c miotest.h
	gcc -g -Imio -o client client.c mio/mio.o mio/mymalloc.o fsema.o

sema.o: fsema.c fsema.h
	gcc -c fsema.c

mio/mio.o: mio/mio.c mio/mio.h
	cd mio; make
