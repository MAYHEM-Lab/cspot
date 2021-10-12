CC=gcc
INC=mymalloc.h
CFLAGS=-g 

LIBS=mymalloc.o

all: mio-test mio-leak-test mio-write-test

mio.o: mio.c ${INC} mio.h
	${CC} ${CFLAGS} -c mio.c

mymalloc.o: mymalloc.c mymalloc.h
	${CC} ${CFLAGS} -c mymalloc.c

mio-test: mio-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-test mio-test.c mio.o ${LIBS}

mio-write-test: mio-write-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-write-test mio-write-test.c mio.o ${LIBS}

mio-leak-test: mio-leak-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-leak-test mio-leak-test.c mio.o ${LIBS}

clean:
	rm *.o mio-test
