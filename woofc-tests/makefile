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

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: woofc-test1

woofc-test1: woofc-test1.c ${WINC} ${WOBJ}
	${CC} ${CFLAGS} -o woofc-test1 woofc-test1.c ${WOBJ} ${SLIB} ${MLIB} ${LIBS}

clean:
	rm -f woofc-test1

