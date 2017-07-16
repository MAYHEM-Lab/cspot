CC=gcc
UINC=../../euca-cutils
MINC=../../mio
SINC=../
ULIB=../../euca-cutils/libutils.a
MLIB=../../mio/mio.o ../../mio/mymalloc.o
SLIB=../lsema.o
LIBS=-lpthread -lm
LOBJ=../log.o ../host.o ../event.o
LINC=../log.h ../host.h ../event.h
WINC=../woofc.h
WOBJ=../woofc.o
WHINC=../woofc-host.h
WHOBJ=../woofc-host.o
TINC=../woofc-thread.h
TOBJ=../woofc-thread.o
SHEP_SRC=../woofc-shepherd.c

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: c-runstest

c-runstest: c-runstest.c c-twist.o c-twist.h
	${CC} ${CFLAGS} -o c-runstest c-runstest.c c-twist.o ${LIBS}

c-twist.o: c-twist.c c-twist.h
	${CC} ${CFLAGS} -c c-twist.c

clean:
	rm -f *.o c-runstest

