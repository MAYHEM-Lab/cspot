CC=gcc
UINC=../euca-cutils
MINC=../mio
SINC=./
ULIB=../euca-cutils/libutils.a
MLIB=../mio/mio.o ../mio/mymalloc.o
SLIB=./lsema.o
LIBS=-lpthread -lm
LOBJ=log.o host.o event.o
LINC=log.h host.h event.h
WINC=woofc.h
WOBJ=woofc.o
WHOBJ=woofc-host.h
WHOBJ=woofc-host.o
TINC=woofc-thread.h
TOBJ=woofc-thread.o

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: log-test log-test-thread woofc.o woofc-host.o woofc-shepherd.o woofc-container

log-test: ${LOBJ} ${LINC} log-test.c ${SLIB}
	${CC} ${CFLAGS} -o log-test log-test.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log-test-thread: ${LOBJ} ${LINC} log-test-thread.c ${SLIB}
	${CC} ${CFLAGS} -o log-test-thread log-test-thread.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log.o: log.c log.h host.h event.h
	${CC} ${CFLAGS} -c log.c

woofc.o: woofc.c woofc.h
	${CC} ${CFLAGS} -c woofc.c

woofc-host.o: woofc-host.c woofc.h
	${CC} ${CFLAGS} -c woofc-host.c

woofc-shepherd.o: woofc-shepherd.c woofc.h
	${CC} ${CFLAGS} -c woofc-shepherd.c

woofc-thread.o: woofc-thread.c woofc-thread.h
	${CC} ${CFLAGS} -c woofc-thread.c

woofc-container: woofc-container.c ${LOBJ} ${WOBJ} ${SLIB} ${WINC}
	${CC} ${CFLAGS} woofc-container.c -o woofc-container ${MLIB} ${LOBJ} ${WOBJ} ${SLIB} ${ULIB} ${LIBS}

event.o: event.c event.h
	${CC} ${CFLAGS} -c event.c

host.o: host.c host.h event.h
	${CC} ${CFLAGS} -c host.c

sema.o: sema.c sema.h
	${CC} ${CFLAGS} -c sema.c

lsema.o: lsema.c lsema.h
	${CC} ${CFLAGS} -c lsema.c

clean:
	rm -f *.o log-test

