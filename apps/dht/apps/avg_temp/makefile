CC=gcc
DEP=../../../../..
WOOFC=../../../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq -lcrypto
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o ${WOOFC}/repair.o
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHOBJ=${WOOFC}/woofc-host.o 
SHEP_SRC=${WOOFC}/woofc-shepherd.c

DHTDIR=../..
DHTOBJ=${DHTDIR}/dht.o ${DHTDIR}/dht_utils.o ${DHTDIR}/dht_client.o
RAFTDIR=${WOOFC}/apps/raft
RAFTOBJ=${RAFTDIR}/raft.o ${RAFTDIR}/raft_client.o
MONITORINC=${WOOFC}/apps/monitor/
MONITOROBJ=${WOOFC}/apps/monitor/monitor.o

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${DHTDIR} -I${RAFTDIR} -I${MONITORINC} -DUSE_RAFT

all: h_avg_temp init_avg_topic init_room_topic latest_avg_temp publish_temp

h_avg_temp: h_avg_temp.c avg_temp.h
	sed 's/PUT_HANDLER_NAME/h_avg_temp/g' ${DHTDIR}/handler_wrapper.c > h_avg_temp_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_avg_temp_shepherd.c
	${CC} ${CFLAGS} -c h_avg_temp_shepherd.c -o h_avg_temp_shepherd.o
	${CC} ${CFLAGS} -c h_avg_temp.c -o h_avg_temp.o
	${CC} ${CFLAGS} -o h_avg_temp h_avg_temp_wrapped.c h_avg_temp.o h_avg_temp_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_avg_topic: init_avg_topic.c avg_temp.h
	${CC} ${CFLAGS} -o init_avg_topic init_avg_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_room_topic: init_room_topic.c avg_temp.h
	${CC} ${CFLAGS} -o init_room_topic init_room_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

latest_avg_temp: latest_avg_temp.c avg_temp.h
	${CC} ${CFLAGS} -o latest_avg_temp latest_avg_temp.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

publish_temp: publish_temp.c avg_temp.h
	${CC} ${CFLAGS} -o publish_temp publish_temp.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf *.o *_wrapped.c *_shepherd.c h_avg_temp init_avg_topic init_room_topic latest_avg_temp publish_temp
