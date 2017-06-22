CC=gcc
UINC=../euca-cutils
MINC=../mio
SINC=./
ULIB=../euca-cutils/libutils.a
MLIB=../mio/mio.o ../mio/mymalloc.o
SLIB=./sema.o
LIBS=-lpthread -lm
LOBJ=log.o host.o event.o
LINC=log.h host.h event.h
WINC=woofc.h
WOBJ=woofc.o

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: log-test log-test-thread woofc-test1

log-test: ${LOBJ} ${LINC} log-test.c ${SLIB}
	${CC} ${CFLAGS} -o log-test log-test.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log-test-thread: ${LOBJ} ${LINC} log-test-thread.c ${SLIB}
	${CC} ${CFLAGS} -o log-test-thread log-test-thread.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

woofc-test1: woofc-test1.c ${WINC} ${WOBJ}
	${CC} ${CFLAGS} -o woofc-test1 woofc-test1.c ${WOBJ} ${SLIB} ${MLIB} ${LIBS}

log.o: log.c log.h host.h event.h
	${CC} ${CFLAGS} -c log.c

woofc.o: woofc.c woofc.h
	${CC} ${CFLAGS} -c woofc.c

event.o: event.c event.h
	${CC} ${CFLAGS} -c event.c

host.o: host.c host.h event.h
	${CC} ${CFLAGS} -c host.c

sema.o: sema.c sema.h
	${CC} ${CFLAGS} -c sema.c

clean:
	rm -f *.o log-test

