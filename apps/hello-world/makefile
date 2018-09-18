CC=gcc
CXX=g++
DEP=../../..
WOOFC=../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq -lstdc++
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o ${WOOFC}/msgpack-ep.o ${WOOFC}/cwpack/cwpack.o
WHINC=${WOOFC}/woofc-host.h
WHOBJ=${WOOFC}/woofc-host.o
TINC=${WOOFC}/woofc-thread.h
TOBJ=${WOOFC}/woofc-thread.o
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=hw

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: hw-start hw-client ${HAND1}

hw-start: hw-start.c hw.h
	${CC} ${CFLAGS} -c -o hw-start.o hw-start.c
	${CC} ${CFLAGS} -o hw-start hw-start.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp hw-start ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

hw-client: hw-client.c hw.h
	${CC} ${CFLAGS} -c -o hw-client.o hw-client.c
	${CC} ${CFLAGS} -o hw-client hw-client.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -DWOOF_HANDLER_NAME=${HAND1} -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -c -o ${HAND1}.o ${HAND1}.c
#	${CXX} ${CFLAGS} -o ${HAND1} ${HAND1}.o ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.o ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS} 
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

clean:
	rm -f hw-start hw-client ${HAND1}

