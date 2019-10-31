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

all: app workload-generator 

workload-generator: workload-generator.c
	${CC} ${CFLAGS} -o workload-generator workload-generator.c

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

clean:
	rm -f app *.o  

