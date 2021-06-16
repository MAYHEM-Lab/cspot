CC=gcc
DEP=../../../..
WOOFC=../../..
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
PYINC=/usr/include/python2.7/
PYLIB=/usr/lib/python2.7/ -lpython2.7
HAND1=ts

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${PYINC} -L${PYLIB}

all: ${HAND1}

${HAND1}: ${HAND1}.pyx ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	python setup.py build_ext --inplace
	sed 's/PYTHON_MODULE/${HAND1}/g' ../../python_handler.c > python_handler.c 
	sed 's/WOOF_HANDLER_NAME/python_handler/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} python_handler.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../../woofc-container ./cspot; cp ../../../woofc-namespace-platform ./cspot; cp ./${HAND1}.so ./cspot; cp start.py ./cspot; cp ../../libwoof.so ./cspot; cp woof.py ./cspot;

clean:
	rm -rf ${HAND1} *.o *.pyc *.so ./cspot python_handler.c ${HAND1}_shepherd.c ts.c build

