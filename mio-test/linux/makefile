all: client client-1

client: mio/mio.o mio/mio.h mio/mymalloc.o lsema.o lsema.h client.c miotest.h
	gcc -g -Imio -o client client.c mio/mio.o mio/mymalloc.o lsema.o -lpthread

client-1: mio/mio.o mio/mio.h mio/mymalloc.o lsema.o lsema.h client-1.c miotest.h
	gcc -g -Imio -o client-1 client-1.c mio/mio.o mio/mymalloc.o lsema.o -lpthread

lsema.o: lsema.c lsema.h
	gcc -c lsema.c

mio/mio.o: mio/mio.c mio/mio.h
	cd mio; make
