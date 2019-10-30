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
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o ${WOOFC}/repair.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h ${WOOFC}/repair.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h 
WHOBJ=${WOOFC}/woofc-host.o 
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=DataHandler
HAND2=AccessPointerHandler
HAND3=LinkHandler

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: app ${HAND1} ${HAND2} ${HAND3}

app: app.c Data.o Link.o Helper.o DataItem.o BST.o
	${CC} ${CFLAGS} -o app app.c Data.o Link.o Helper.o DataItem.o BST.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp app ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
Data.o: Data.c
	${CC} -c Data.c
Link.o: Link.c
	${CC} -c Link.c
Helper.o: Helper.c
	${CC} ${CFLAGS} -c Helper.c
DataItem.o: DataItem.c
	${CC} -c DataItem.c
BST.o: BST.c
	${CC} ${CFLAGS} -c BST.c Helper.o

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} DataItem.o Helper.o ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

${HAND2}: ${HAND2}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND2}/g' ${SHEP_SRC} > ${HAND2}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND2}_shepherd.c -o ${HAND2}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND2} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

${HAND3}: ${HAND3}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND3}/g' ${SHEP_SRC} > ${HAND3}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND3}_shepherd.c -o ${HAND3}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND3} ${HAND3}.c ${HAND3}_shepherd.o ${WOBJ} DataItem.o ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND3} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

clean:
	rm -f app ${HAND1} ${HAND2} ${HAND3} *.o

