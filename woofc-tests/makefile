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

HAND1=woofc_obj1_handler_1

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: woofc-test1 ${HAND1} woofc-obj1-test

woofc-test1: woofc-test1.c ${TINC} ${TOBJ}
	${CC} ${CFLAGS} -o woofc-test1 woofc-test1.c ${TOBJ} ${SLIB} ${MLIB} ${LIBS}

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -DWOOF_HANDLER_NAME=${HAND1} -o ${HAND1} ${HAND1}.c ${SHEP_SRC} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	cp ${HAND1} ./cspot

woofc-obj1-test: woofc-obj1-test.c ${WINC} ${WHINC} ${LINC} ${LOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -o woofc-obj1-test woofc-obj1-test.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	cp woofc-obj1-test ./cspot

clean:
	rm -f woofc-test1

