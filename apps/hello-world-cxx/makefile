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
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h
WHOBJ=${WOOFC}/woofc-host.o
TINC=${WOOFC}/woofc-thread.h
TOBJ=${WOOFC}/woofc-thread.o
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=hw

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}
CXX_FLAGS= -std=c++11 -g -I${UINC} -I${MINC} -I${SINC}

all: hw-start ${HAND1}

hw-start: hw-start.cpp hw.hpp
	${CXX} ${CXX_FLAGS} -o hw-start hw-start.cpp ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp hw-start ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

${HAND1}: ${HAND1}.cpp ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -c ${SHEP_SRC} -DWOOF_HANDLER_NAME=${HAND1} -o ${HAND1}_shepherd.o
	${CXX} ${CXX_FLAGS} -o ${HAND1} ${HAND1}.cpp ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

clean:
	rm -f hw-start hw-client ${HAND1}

