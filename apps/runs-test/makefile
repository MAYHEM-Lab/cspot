CC=gcc
UINC=../../../euca-cutils
MINC=../../../mio
SINC=../..
ULIB=../../../euca-cutils/libutils.a
MLIB=../../../mio/mio.o ../../mio/mymalloc.o
SLIB=../../lsema.o
LIBS=-lpthread -lm
LOBJ=../../log.o ../host.o ../event.o
LINC=../../log.h ../host.h ../event.h
WINC=../..
WOBJ=../../woofc.o
WHINC=../..
WHOBJ=../../woofc-host.o
TINC=../..
TOBJ=../../woofc-thread.o
KINC=ks
KLIB=ks/kslib.a
SHEP_SRC=../../woofc-shepherd.c

CFLAGS=-g -I${SINC} -I${KINC} -I${MINC} -I${UINC}

all: c-runstest c-runstat

c-runstest: c-runstest.c c-runstest.h c-twist.o c-twist.h normal.o ${KLIB}
	${CC} ${CFLAGS} -DSTANDALONE -o c-runstest c-runstest.c c-twist.o ${KLIB} normal.o ${LIBS}

c-runstat: c-runstat.c c-runstest.o c-runstest.h c-twist.o c-twist.h
	${CC} ${CFLAGS} -o c-runstat c-runstat.c c-runstest.o c-twist.o ${KLIB} normal.o ${LIBS}

c-runstest.o: c-runstest.c c-runstest.h
	${CC} ${CFLAGS} -c c-runstest.c ${LIBS}

c-runstest-cspot.o: c-runstest.c c-runstest.h
	${CC} ${CFLAGS} -DHAS_WOOF -c c-runstest.c -o c-runstest-cspot.o ${LIBS}

c-twist.o: c-twist.c c-twist.h
	${CC} ${CFLAGS} -c c-twist.c

normal.o: normal.c normal.h
	${CC} ${CFLAGS} -c normal.c
	

${KLIB}:
	cd ks; make

clean:
	rm -f *.o c-runstest

