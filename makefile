CC=gcc
UINC=../euca-cutils
MINC=../mio
ULIB=../euca-cutils/libutils.a
MLIB=../mio/mio.o ../mio/mymalloc.o
LIBS=-lpthread -lm
LOBJ=log.o host.o event.o
LINC=log.h host.h event.h

CFLAGS=-g -I${UINC} -I${MINC}

all: log-test

log-test: ${LOBJ} ${LINC} log-test.c
	${CC} ${CFLAGS} -o log-test log-test.c ${LOBJ} ${ULIB} ${MLIB} ${LIBS}

log.o: log.c log.h host.h event.h
	${CC} ${CFLAGS} -c log.c

event.o: event.c event.h
	${CC} ${CFLAGS} -c event.c

host.o: host.c host.h event.h
	${CC} ${CFLAGS} -c host.c

clean:
	rm -f *.o log-test

