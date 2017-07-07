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
SHEP_SRC=../woofc_shepherd.c

WOOFOBJ1=woofc-obj1

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: woofc-test1 ${WOOFOBJ1}.handler woofc-obj1-test

woofc-test1: woofc-test1.c ${WINC} ${WOBJ}
	${CC} ${CFLAGS} -o woofc-test1 woofc-test1.c ${WOBJ} ${SLIB} ${MLIB} ${LIBS}

${WOOFOBJ1}.handler: ${HANDLER1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ}
	${CC} ${CFLAGS} -DWOOF_HANDLER_NAME=${WOOFOBJ1}.handler -o ${HANDLER1}.handler ${HANDLER1}.c ${SHEP_SRC} ${WOBJ} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

woofc-obj1-test: woofc-obj1-test.c ${WINC} ${LINC} ${LOBJ} ${WOBJ}
	${CC} ${CFLAGS} -o woofc-obj1-test woofc-obj1-test.c ${WOBJ} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -f woofc-test1

