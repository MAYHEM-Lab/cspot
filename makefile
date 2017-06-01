CC=gcc
UINC=../euca-cutils
ULIB=../libutils.a
LIBS=-lpthread
LOBJ=log.o host.o event.o
LINC=log.h host.h event.h

CFLAGS=-g -I${UINC}

all: log-test

log-test: ${LOBJ} ${LINC} log-test.c
	${CC} ${CFLAGS} -o log-test log-test.c ${LOBJ} ${ULIB} ${LIBS}

log.o: log.c log.h host.h event.h
	${CC} ${CFLAGS} -c log.c

event.o: event.c event.h
	${CC} ${CFLAGS} -c event.c

host.o: host.c host.h event.h
	${CC} ${CFLAGS} -c host.c


