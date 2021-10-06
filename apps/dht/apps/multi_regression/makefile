CC=gcc
CXX=g++ -std=c++11
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

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${DHTDIR} -I${RAFTDIR} -I${MONITORINC}
BOOSTFLAGS=-I/usr/include/boost169 -lmlpack -fopenmp

all: h_predict h_smooth h_train init_topic list_error publish_temp

h_predict: h_predict.cpp multi_regression.h
	sed 's/PUT_HANDLER_NAME/h_predict/g' ${DHTDIR}/handler_wrapper.c > h_predict_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_predict_shepherd.c
	${CC} ${CFLAGS} -c h_predict_shepherd.c -o h_predict_shepherd.o
	${CC} ${CFLAGS} -c h_predict_wrapped.c -o h_predict_wrapped.o
	${CXX} ${CFLAGS} ${BOOSTFLAGS} -c h_predict.cpp -o h_predict.o
	${CXX} ${CFLAGS} ${BOOSTFLAGS} -o h_predict h_predict_wrapped.o h_predict.o h_predict_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_smooth: h_smooth.cpp multi_regression.h
	sed 's/PUT_HANDLER_NAME/h_smooth/g' ${DHTDIR}/handler_wrapper.c > h_smooth_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_smooth_shepherd.c
	${CC} ${CFLAGS} -c h_smooth_shepherd.c -o h_smooth_shepherd.o
	${CC} ${CFLAGS} -c h_smooth_wrapped.c -o h_smooth_wrapped.o
	${CXX} ${CFLAGS} ${BOOSTFLAGS} -c h_smooth.cpp -o h_smooth.o
	${CXX} ${CFLAGS} ${BOOSTFLAGS} -o h_smooth h_smooth_wrapped.o h_smooth.o h_smooth_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_train: h_train.cpp multi_regression.h
	sed 's/PUT_HANDLER_NAME/h_train/g' ${DHTDIR}/handler_wrapper.c > h_train_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > h_train_shepherd.c
	${CC} ${CFLAGS} -c h_train_shepherd.c -o h_train_shepherd.o
	${CC} ${CFLAGS} -c h_train_wrapped.c -o h_train_wrapped.o
	${CXX} ${CFLAGS} ${BOOSTFLAGS} -c h_train.cpp -o h_train.o
	${CXX} ${CFLAGS} ${BOOSTFLAGS} -o h_train h_train_wrapped.o h_train.o h_train_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_topic: init_topic.c multi_regression.h
	${CC} ${CFLAGS} -o init_topic init_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

list_error: list_error.c multi_regression.h
	${CC} ${CFLAGS} -o list_error list_error.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

publish_temp: publish_temp.c multi_regression.h
	${CC} ${CFLAGS} -o publish_temp publish_temp.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf *.o *_wrapped.c *_shepherd.c h_predict h_smooth h_train init_topic list_error publish_temp
