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
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o ${WOOFC}/repair.o 
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h ${WOOFC}/repair.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h
WHOBJ=${WOOFC}/woofc-host.o
TINC=${WOOFC}/woofc-thread.h
TOBJ=${WOOFC}/woofc-thread.o
SHEP_SRC=${WOOFC}/woofc-shepherd.c

STATINC=regress-matrix.h ssa-decomp.h
STATOBJ=regress-matrix.o ssa-decomp.o

HAND1=RegressPairReqHandler
HAND2=RegressPairPredictedHandler
HAND3=RegressPairMeasuredHandler

CFLAGS=-g -I ${DINC} -I${UINC} -I${MINC} -I${SINC} -I${MXINC} -L${LALIBPATH} -L/usr/local/Cellar/gcc/7.2.0/lib/gcc/7/ -L/usr/lib64/atlas

all: regress-pair-put regress-pair-get regress-pair-init regress-pair-update ${HAND1} ${HAND2} ${HAND3} ssa-decomp
	mkdir -p cspot; cp test-data/* cspot
	mkdir -p cspot-2; cp test-data/* cspot-2

regress-pair-put: regress-pair-put.c regress-pair.h
	${CC} ${CFLAGS} -o regress-pair-put regress-pair-put.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp regress-pair-put ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp regress-pair-put ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

regress-pair-get: regress-pair-get.c regress-pair.h
	${CC} ${CFLAGS} -o regress-pair-get regress-pair-get.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp regress-pair-get ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp regress-pair-get ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

regress-pair-init: regress-pair-init.c regress-pair.h
	${CC} ${CFLAGS} -o regress-pair-init regress-pair-init.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp regress-pair-init ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp regress-pair-init ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

regress-pair-update: regress-pair-update.c regress-pair.h
	${CC} ${CFLAGS} -o regress-pair-update regress-pair-update.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp regress-pair-update ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp regress-pair-update ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

regress-matrix.o: regress-matrix.c regress-matrix.h ${MXINC} ${MXLIB}
	${CC} ${CFLAGS} -c regress-matrix.c

ssa-decomp.o: ssa-decomp.c ssa-decomp.h ${MXINC} ${MXLIB}
	${CC} ${CFLAGS} -c ssa-decomp.c

ssa-decomp: ssa-decomp.c ssa-decomp.h ${MXINC} ${MXLIB}
	${CC} ${CFLAGS} -DSTANDALONE ssa-decomp.c ${DLIB} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS} ${MXLIB} ${LALIB}

${HAND1}: regress-pair.h ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND1} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND2}: regress-pair.h ${HAND2}.c ${STATINC} ${STATOBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC} ${MXINC}
	sed 's/WOOF_HANDLER_NAME/${HAND2}/g' ${SHEP_SRC} > ${HAND2}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND2}_shepherd.c -o ${HAND2}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${STATOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${MXLIB} ${DLIB} ${LALIB} ${LIBS}
	mkdir -p cspot; cp ${HAND2} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND2} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND3}: regress-pair.h ${HAND3}.c ${STATINC} ${STATOBJ} regress-matrix.h ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC} ${MXINC}
	sed 's/WOOF_HANDLER_NAME/${HAND3}/g' ${SHEP_SRC} > ${HAND3}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND3}_shepherd.c -o ${HAND3}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND3} ${HAND3}.c ${HAND3}_shepherd.o ${STATOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${MXLIB} ${DLIB} ${LALIB} ${LIBS}
	mkdir -p cspot; cp ${HAND3} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND3} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

clean:
	rm -f regress-pair-init regress-pair-put regress-pair-get regress-pair-update *.o ${HAND1} ${HAND2}
	rm -rf cspot cspot-2

