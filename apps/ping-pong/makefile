CC=gcc
DEP=../../..
WOOFC=../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h
WHOBJ=${WOOFC}/woofc-host.o
TINC=${WOOFC}/woofc-thread.h
TOBJ=${WOOFC}/woofc-thread.o
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=ping
HAND2=pong

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: ping-pong-start ping-start pong-start ${HAND1} ${HAND2}

ping-pong-start: ping-pong-start.c ping-pong.h
	${CC} ${CFLAGS} -o ping-pong-start ping-pong-start.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ping-pong-start ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ping-pong-start ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

ping-start: ping-start.c ping-pong.h
	${CC} ${CFLAGS} -o ping-start ping-start.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ping-start ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ping-start ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

pong-start: pong-start.c ping-pong.h
	${CC} ${CFLAGS} -o pong-start pong-start.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp pong-start ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp pong-start ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND1} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND2}: ${HAND2}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND2}/g' ${SHEP_SRC} > ${HAND2}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND2}_shepherd.c -o ${HAND2}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND2} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND2} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

clean:
	rm -f ping-pong-start ping-start pong-start *.o ${HAND1} ${HAND2}

