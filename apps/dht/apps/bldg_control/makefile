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

all: h_bldg_total h_floor_total h_hourly_change init_1f_total init_2f_total init_bldg_gate init_bldg_total init_hourly_change init_room_101 init_room_102 init_room_103 init_room_201 init_room_202 show traffic

h_bldg_total: h_bldg_total.c bldg_control.h
	sed 's/PUT_HANDLER_NAME/h_bldg_total/g' ${DHTDIR}/handler_wrapper.c > h_bldg_total_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_bldg_total_shepherd.c
	${CC} ${CFLAGS} -c h_bldg_total_shepherd.c -o h_bldg_total_shepherd.o
	${CC} ${CFLAGS} -c h_bldg_total.c -o h_bldg_total.o
	${CC} ${CFLAGS} -o h_bldg_total h_bldg_total_wrapped.c h_bldg_total.o h_bldg_total_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_floor_total: h_floor_total.c bldg_control.h
	sed 's/PUT_HANDLER_NAME/h_floor_total/g' ${DHTDIR}/handler_wrapper.c > h_floor_total_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_floor_total_shepherd.c
	${CC} ${CFLAGS} -c h_floor_total_shepherd.c -o h_floor_total_shepherd.o
	${CC} ${CFLAGS} -c h_floor_total.c -o h_floor_total.o
	${CC} ${CFLAGS} -o h_floor_total h_floor_total_wrapped.c h_floor_total.o h_floor_total_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_hourly_change: h_hourly_change.c bldg_control.h
	sed 's/PUT_HANDLER_NAME/h_hourly_change/g' ${DHTDIR}/handler_wrapper.c > h_hourly_change_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_hourly_change_shepherd.c
	${CC} ${CFLAGS} -c h_hourly_change_shepherd.c -o h_hourly_change_shepherd.o
	${CC} ${CFLAGS} -c h_hourly_change.c -o h_hourly_change.o
	${CC} ${CFLAGS} -o h_hourly_change h_hourly_change_wrapped.c h_hourly_change.o h_hourly_change_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_1f_total: init_1f_total.c bldg_control.h
	${CC} ${CFLAGS} -o init_1f_total init_1f_total.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_2f_total: init_2f_total.c bldg_control.h
	${CC} ${CFLAGS} -o init_2f_total init_2f_total.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_bldg_gate: init_bldg_gate.c bldg_control.h
	${CC} ${CFLAGS} -o init_bldg_gate init_bldg_gate.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_bldg_total: init_bldg_total.c bldg_control.h
	${CC} ${CFLAGS} -o init_bldg_total init_bldg_total.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_hourly_change: init_hourly_change.c bldg_control.h
	${CC} ${CFLAGS} -o init_hourly_change init_hourly_change.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_room_101: init_room_101.c bldg_control.h
	${CC} ${CFLAGS} -o init_room_101 init_room_101.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_room_102: init_room_102.c bldg_control.h
	${CC} ${CFLAGS} -o init_room_102 init_room_102.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_room_103: init_room_103.c bldg_control.h
	${CC} ${CFLAGS} -o init_room_103 init_room_103.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_room_201: init_room_201.c bldg_control.h
	${CC} ${CFLAGS} -o init_room_201 init_room_201.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_room_202: init_room_202.c bldg_control.h
	${CC} ${CFLAGS} -o init_room_202 init_room_202.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

show: show.c bldg_control.h
	${CC} ${CFLAGS} -o show show.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

traffic: traffic.c bldg_control.h
	${CC} ${CFLAGS} -o traffic traffic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf *.o *_wrapped.c *_shepherd.c h_bldg_total h_floor_total h_hourly_change init_1f_total init_2f_total init_bldg_gate init_bldg_total init_hourly_change init_room_101 init_room_102 init_room_103 init_room_201 init_room_202 show traffic
