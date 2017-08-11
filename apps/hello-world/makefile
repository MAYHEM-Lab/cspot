CC=gcc
UINC=../../../euca-cutils
MINC=../../../mio
SINC=../
ULIB=../../../euca-cutils/libutils.a
MLIB=../../../mio/mio.o ../../mio/mymalloc.o
SLIB=../../lsema.o
LIBS=../../uriparser2/liburiparser2.a -lpthread -lm -lczmq
LOBJ=../../log.o ../host.o ../event.o
LINC=../../log.h ../host.h ../event.h
WINC=../../woofc.h ../woofc-access.h
WOBJ=../../woofc.o ../woofc-access.o
WHINC=../../woofc-host.h
WHOBJ=../../woofc-host.o
TINC=../../woofc-thread.h
TOBJ=../../woofc-thread.o
SHEP_SRC=../../woofc-shepherd.c

HAND1=hw

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: hw-start ${HAND1}

hw-start: hw-start.c hw.h
	${CC} ${CFLAGS} -o hw-start hw-start.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

clean:
	rm -f hw-start ${HAND1}

