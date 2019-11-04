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

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: start_master start_slave replicate_event replay_event

start_master: start_master.c
	${CC} ${CFLAGS} -o start_master start_master.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp start_master ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

start_slave: start_slave.c
	${CC} ${CFLAGS} -o start_slave start_slave.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp start_slave ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

replicate_event: replicate_event.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC} ${LMINC}
	sed 's/WOOF_HANDLER_NAME/replicate_event/g' ${SHEP_SRC} > replicate_event_shepherd.c
	${CC} ${CFLAGS} -c replicate_event_shepherd.c -o replicate_event_shepherd.o
	${CC} ${CFLAGS} -o replicate_event replicate_event.c replicate_event_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp replicate_event ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

replay_event: replay_event.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC} ${LMINC}
	sed 's/WOOF_HANDLER_NAME/replay_event/g' ${SHEP_SRC} > replay_event_shepherd.c
	${CC} ${CFLAGS} -c replay_event_shepherd.c -o replay_event_shepherd.o
	${CC} ${CFLAGS} -o replay_event replay_event.c replay_event_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp replay_event ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

clean:
	rm -f start_master start_slave replicate_event replay_event
