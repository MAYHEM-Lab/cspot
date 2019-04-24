CC=gcc
DEP=../../..
WOOFC=../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
LAINC=${DEP}/matrix/lapack-3.8.0/LAPACKE/include
MXINC=${DEP}/matrix
DINC=${DEP}/distributions
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
MXLIB=${DEP}/matrix/mioregress.o ${DEP}/matrix/mioarray.o
DLIB=${DEP}/distributions/normal.o
LALIBPATH=${DEP}/matrix/lapack-3.8.0
LALIB=-llapacke -llapack -ltatlas -lgfortran
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq -lpthread -ltatlas
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h
WHOBJ=${WOOFC}/woofc-host.o
TINC=${WOOFC}/woofc-thread.h
TOBJ=${WOOFC}/woofc-thread.o
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=MSPulseHandler
HAND2=MSPingPongHandler

CFLAGS=-g -I ${DINC} -I${UINC} -I${MINC} -I${SINC} -I${MXINC} -L${LALIBPATH} -L/usr/local/Cellar/gcc/7.2.0/lib/gcc/7/ -L/usr/lib64/atlas

all: master-slave-init master-slave-pulse ${HAND1} ${HAND2}

master-slave-init: master-slave-init.c master-slave.h
	${CC} ${CFLAGS} -o master-slave-init master-slave-init.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp master-slave-init ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp master-slave-init ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

master-slave-pulse: master-slave-pulse.c master-slave.h
	${CC} ${CFLAGS} -o master-slave-pulse master-slave-pulse.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp master-slave-pulse ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp master-slave-pulse ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND1}: master-slave.h ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND1} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND2}: master-slave.h ${HAND2}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND2}/g' ${SHEP_SRC} > ${HAND2}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND2}_shepherd.c -o ${HAND2}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND2} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND2} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

clean:
	rm -f master-slave-init ${HAND1} ${HAND2} *_shepherd.c

