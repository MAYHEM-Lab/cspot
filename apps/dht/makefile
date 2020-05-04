CC=gcc
DEP=../../..
WOOFC=../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq -lcrypto
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o ${WOOFC}/repair.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h ${WOOFC}/repair.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h 
WHOBJ=${WOOFC}/woofc-host.o 
SHEP_SRC=${WOOFC}/woofc-shepherd.c

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: handlers copy clients test sub_test test2
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp test ./cspot;
	cp sub_test ./cspot;
	cp test2 ./cspot;

copy: handlers clients
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp h_* ./cspot
	cp create ./cspot;
	cp dht_daemon ./cspot;
	cp find ./cspot;
	cp node_info ./cspot;
	cp join ./cspot;

handlers: h_find_result h_find_successor h_fix_fingers_callback h_get_predecessor h_init_topic h_join_callback h_notify_callback h_notify h_stablize_callback h_subscribe h_trigger 

clients: create dht_daemon find node_info join

create: create.c dht.o
	${CC} ${CFLAGS} -o create create.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

dht_daemon: dht_daemon.c dht.o
	${CC} ${CFLAGS} -o dht_daemon dht_daemon.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

find: find.c dht.o
	${CC} ${CFLAGS} -o find find.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

node_info: node_info.c dht.o
	${CC} ${CFLAGS} -o node_info node_info.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

join: join.c dht.o
	${CC} ${CFLAGS} -o join join.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

dht.o: dht.c dht.h
	${CC} ${CFLAGS} -c dht.c

h_find_result: h_find_result.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_find_result/g' ${SHEP_SRC} > h_find_result_shepherd.c
	${CC} ${CFLAGS} -c h_find_result_shepherd.c -o h_find_result_shepherd.o
	${CC} ${CFLAGS} -o h_find_result h_find_result.c h_find_result_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_find_successor: h_find_successor.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_find_successor/g' ${SHEP_SRC} > h_find_successor_shepherd.c
	${CC} ${CFLAGS} -c h_find_successor_shepherd.c -o h_find_successor_shepherd.o
	${CC} ${CFLAGS} -o h_find_successor h_find_successor.c h_find_successor_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_fix_fingers_callback: h_fix_fingers_callback.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_fix_fingers_callback/g' ${SHEP_SRC} > h_fix_fingers_callback_shepherd.c
	${CC} ${CFLAGS} -c h_fix_fingers_callback_shepherd.c -o h_fix_fingers_callback_shepherd.o
	${CC} ${CFLAGS} -o h_fix_fingers_callback h_fix_fingers_callback.c h_fix_fingers_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_get_predecessor: h_get_predecessor.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_get_predecessor/g' ${SHEP_SRC} > h_get_predecessor_shepherd.c
	${CC} ${CFLAGS} -c h_get_predecessor_shepherd.c -o h_get_predecessor_shepherd.o
	${CC} ${CFLAGS} -o h_get_predecessor h_get_predecessor.c h_get_predecessor_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_init_topic: h_init_topic.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_init_topic/g' ${SHEP_SRC} > h_init_topic_shepherd.c
	${CC} ${CFLAGS} -c h_init_topic_shepherd.c -o h_init_topic_shepherd.o
	${CC} ${CFLAGS} -o h_init_topic h_init_topic.c h_init_topic_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_join_callback: h_join_callback.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_join_callback/g' ${SHEP_SRC} > h_join_callback_shepherd.c
	${CC} ${CFLAGS} -c h_join_callback_shepherd.c -o h_join_callback_shepherd.o
	${CC} ${CFLAGS} -o h_join_callback h_join_callback.c h_join_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_notify_callback: h_notify_callback.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_notify_callback/g' ${SHEP_SRC} > h_notify_callback_shepherd.c
	${CC} ${CFLAGS} -c h_notify_callback_shepherd.c -o h_notify_callback_shepherd.o
	${CC} ${CFLAGS} -o h_notify_callback h_notify_callback.c h_notify_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_notify: h_notify.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_notify/g' ${SHEP_SRC} > h_notify_shepherd.c
	${CC} ${CFLAGS} -c h_notify_shepherd.c -o h_notify_shepherd.o
	${CC} ${CFLAGS} -o h_notify h_notify.c h_notify_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_stablize_callback: h_stablize_callback.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_stablize_callback/g' ${SHEP_SRC} > h_stablize_callback_shepherd.c
	${CC} ${CFLAGS} -c h_stablize_callback_shepherd.c -o h_stablize_callback_shepherd.o
	${CC} ${CFLAGS} -o h_stablize_callback h_stablize_callback.c h_stablize_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_subscribe: h_subscribe.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_subscribe/g' ${SHEP_SRC} > h_subscribe_shepherd.c
	${CC} ${CFLAGS} -c h_subscribe_shepherd.c -o h_subscribe_shepherd.o
	${CC} ${CFLAGS} -o h_subscribe h_subscribe.c h_subscribe_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_trigger: h_trigger.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_trigger/g' ${SHEP_SRC} > h_trigger_shepherd.c
	${CC} ${CFLAGS} -c h_trigger_shepherd.c -o h_trigger_shepherd.o
	${CC} ${CFLAGS} -o h_trigger h_trigger.c h_trigger_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

sub_test: sub_test.c dht.o
	${CC} ${CFLAGS} -o sub_test sub_test.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test: test.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test/g' ${SHEP_SRC} > test_shepherd.c
	${CC} ${CFLAGS} -c test_shepherd.c -o test_shepherd.o
	${CC} ${CFLAGS} -o test test.c test_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test2: test2.c dht.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test2/g' ${SHEP_SRC} > test2_shepherd.c
	${CC} ${CFLAGS} -c test2_shepherd.c -o test2_shepherd.o
	${CC} ${CFLAGS} -o test2 test2.c test2_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o create dht_daemon find node_info join h_find_result h_find_successor h_fix_fingers_callback h_get_predecessor h_init_topic h_join_callback h_notify_callback h_notify h_stablize_callback h_subscribe h_trigger

