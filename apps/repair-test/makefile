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
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=repair

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: repair-init repair-start ${HAND1}

repair-init: repair-init.c repair.h
	${CC} ${CFLAGS} -o repair-init repair-init.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp repair-init ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

repair-start: repair-start.c repair.h
	${CC} ${CFLAGS} -o repair-start repair-start.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp repair-start ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC} ${LMINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

clean:
	rm -f repair-init repair-start ${HAND1}

