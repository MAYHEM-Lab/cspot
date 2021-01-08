CC=gcc

CSPOTPARENT=../../../
CSPOT=${CSPOTPARENT}/cspot
SHEP_SRC=${CSPOT}/woofc-shepherd.c
WINC=${CSPOT}/woofc.h ${CSPOT}/woofc-access.h ${CSPOT}/woofc-cache.h 
LINC=${CSPOT}/log.h ${CSPOT}/host.h ${CSPOT}/event.h ${CSPOT}/repair.h
LOBJ=${CSPOT}/log.o ${CSPOT}/host.o ${CSPOT}/event.o ${CSPOT}/repair.o
WOBJ=${CSPOT}/woofc.o ${CSPOT}/woofc-access.o ${CSPOT}/woofc-cache.o
WHOBJ=${CSPOT}/woofc-host.o
SLIB=${CSPOT}/lsema.o
LIBS=${CSPOT}/uriparser2/liburiparser2.a -lpthread -lm -lczmq
MLIB=${CSPOTPARENT}/mio/mio.o ${CSPOTPARENT}/mio/mymalloc.o
ULIB=${CSPOTPARENT}/euca-cutils/libutils.a

CFLAGS=-g -Wall -I${CSPOT} -I${CSPOTPARENT}/mio -I${CSPOTPARENT}/euca-cutils

all: app

app: app.c
	${CC} ${CFLAGS} -o $@ $^ ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${LIBS} ${ULIB}
	mkdir -p cspot; cp $@ ./cspot; cp ${CSPOT}/woofc-container ./cspot; cp ${CSPOT}/woofc-namespace-platform ./cspot;

clean:
	rm -rf *.o app
