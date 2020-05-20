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

RAFTDIR=${WOOFC}/apps/raft
RAFTOBJ=${RAFTDIR}/raft.o ${RAFTDIR}/raft_client.o ${WOOFC}/apps/monitor/monitor.o

# CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}
CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${RAFTDIR} -DUSE_RAFT

all: handlers copy clients test

copy: handlers clients test
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp d_check_predecessor ./cspot;
	cp d_daemon ./cspot;
	cp d_fix_finger ./cspot;
	cp d_stablize ./cspot;
	cp h_find_result ./cspot;
	cp h_find_successor ./cspot;
	cp h_fix_finger_callback ./cspot;
	cp h_get_predecessor ./cspot;
	cp h_join_callback ./cspot;
	cp h_notify_callback ./cspot;
	cp h_notify ./cspot;
	cp h_register_topic ./cspot;
	cp h_stablize_callback ./cspot;
	cp h_subscribe ./cspot;
	cp h_trigger ./cspot;
	cp s_create ./cspot;
	cp s_join ./cspot;
	cp client_find_node ./cspot;
	cp client_init_topic ./cspot;
	cp client_node_info ./cspot;
	cp client_publish ./cspot;
	cp client_subscribe ./cspot;
	cp test_handler ./cspot;
	cp test_publish ./cspot;
	cp test_subscribe ./cspot;

handlers: d_check_predecessor d_daemon d_fix_finger d_stablize h_find_result h_find_successor h_fix_finger_callback h_get_predecessor h_join_callback h_notify_callback h_notify h_register_topic h_stablize_callback h_subscribe h_trigger

clients: s_create s_join client_find_node client_init_topic client_node_info client_publish client_subscribe

test: test_handler test_publish test_subscribe

client_find_node: client_find_node.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${RAFTOBJ}
	${CC} ${CFLAGS} -o client_find_node client_find_node.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_init_topic: client_init_topic.c dht.o dht_utils.o dht_client.o
	${CC} ${CFLAGS} -o client_init_topic client_init_topic.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_node_info: client_node_info.c dht.o dht_utils.o dht_client.o
	${CC} ${CFLAGS} -o client_node_info client_node_info.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_publish: client_publish.c dht.o dht_utils.o dht_client.o
	${CC} ${CFLAGS} -o client_publish client_publish.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_subscribe: client_subscribe.c dht.o dht_utils.o dht_client.o
	${CC} ${CFLAGS} -o client_subscribe client_subscribe.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

dht_client.o: dht_client.c dht_client.h
	$(CC) $(CFLAGS) -c dht_client.c
	
dht_utils.o: dht_utils.c dht_utils.h
	${CC} ${CFLAGS} -c dht_utils.c

dht.o: dht.c dht.h
	${CC} ${CFLAGS} -c dht.c

d_check_predecessor: d_check_predecessor.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_check_predecessor/g' ${SHEP_SRC} > d_check_predecessor_shepherd.c
	${CC} ${CFLAGS} -c d_check_predecessor_shepherd.c -o d_check_predecessor_shepherd.o
	${CC} ${CFLAGS} -o d_check_predecessor d_check_predecessor.c d_check_predecessor_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_daemon: d_daemon.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_daemon/g' ${SHEP_SRC} > d_daemon_shepherd.c
	${CC} ${CFLAGS} -c d_daemon_shepherd.c -o d_daemon_shepherd.o
	${CC} ${CFLAGS} -o d_daemon d_daemon.c d_daemon_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_fix_finger: d_fix_finger.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_fix_finger/g' ${SHEP_SRC} > d_fix_finger_shepherd.c
	${CC} ${CFLAGS} -c d_fix_finger_shepherd.c -o d_fix_finger_shepherd.o
	${CC} ${CFLAGS} -o d_fix_finger d_fix_finger.c d_fix_finger_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_stablize: d_stablize.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_stablize/g' ${SHEP_SRC} > d_stablize_shepherd.c
	${CC} ${CFLAGS} -c d_stablize_shepherd.c -o d_stablize_shepherd.o
	${CC} ${CFLAGS} -o d_stablize d_stablize.c d_stablize_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_find_result: h_find_result.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_find_result/g' ${SHEP_SRC} > h_find_result_shepherd.c
	${CC} ${CFLAGS} -c h_find_result_shepherd.c -o h_find_result_shepherd.o
	${CC} ${CFLAGS} -o h_find_result h_find_result.c h_find_result_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_find_successor: h_find_successor.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_find_successor/g' ${SHEP_SRC} > h_find_successor_shepherd.c
	${CC} ${CFLAGS} -c h_find_successor_shepherd.c -o h_find_successor_shepherd.o
	${CC} ${CFLAGS} -o h_find_successor h_find_successor.c h_find_successor_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_fix_finger_callback: h_fix_finger_callback.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_fix_finger_callback/g' ${SHEP_SRC} > h_fix_finger_callback_shepherd.c
	${CC} ${CFLAGS} -c h_fix_finger_callback_shepherd.c -o h_fix_finger_callback_shepherd.o
	${CC} ${CFLAGS} -o h_fix_finger_callback h_fix_finger_callback.c h_fix_finger_callback_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_get_predecessor: h_get_predecessor.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_get_predecessor/g' ${SHEP_SRC} > h_get_predecessor_shepherd.c
	${CC} ${CFLAGS} -c h_get_predecessor_shepherd.c -o h_get_predecessor_shepherd.o
	${CC} ${CFLAGS} -o h_get_predecessor h_get_predecessor.c h_get_predecessor_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_join_callback: h_join_callback.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_join_callback/g' ${SHEP_SRC} > h_join_callback_shepherd.c
	${CC} ${CFLAGS} -c h_join_callback_shepherd.c -o h_join_callback_shepherd.o
	${CC} ${CFLAGS} -o h_join_callback h_join_callback.c h_join_callback_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_notify_callback: h_notify_callback.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_notify_callback/g' ${SHEP_SRC} > h_notify_callback_shepherd.c
	${CC} ${CFLAGS} -c h_notify_callback_shepherd.c -o h_notify_callback_shepherd.o
	${CC} ${CFLAGS} -o h_notify_callback h_notify_callback.c h_notify_callback_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_notify: h_notify.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_notify/g' ${SHEP_SRC} > h_notify_shepherd.c
	${CC} ${CFLAGS} -c h_notify_shepherd.c -o h_notify_shepherd.o
	${CC} ${CFLAGS} -o h_notify h_notify.c h_notify_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_register_topic: h_register_topic.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_register_topic/g' ${SHEP_SRC} > h_register_topic_shepherd.c
	${CC} ${CFLAGS} -c h_register_topic_shepherd.c -o h_register_topic_shepherd.o
	${CC} ${CFLAGS} -o h_register_topic h_register_topic.c h_register_topic_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_stablize_callback: h_stablize_callback.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_stablize_callback/g' ${SHEP_SRC} > h_stablize_callback_shepherd.c
	${CC} ${CFLAGS} -c h_stablize_callback_shepherd.c -o h_stablize_callback_shepherd.o
	${CC} ${CFLAGS} -o h_stablize_callback h_stablize_callback.c h_stablize_callback_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_subscribe: h_subscribe.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_subscribe/g' ${SHEP_SRC} > h_subscribe_shepherd.c
	${CC} ${CFLAGS} -c h_subscribe_shepherd.c -o h_subscribe_shepherd.o
	${CC} ${CFLAGS} -o h_subscribe h_subscribe.c h_subscribe_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_trigger: h_trigger.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_trigger/g' ${SHEP_SRC} > h_trigger_shepherd.c
	${CC} ${CFLAGS} -c h_trigger_shepherd.c -o h_trigger_shepherd.o
	${CC} ${CFLAGS} -o h_trigger h_trigger.c h_trigger_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_create: s_create.c dht.o dht_utils.o
	${CC} ${CFLAGS} -o s_create s_create.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_join: s_join.c dht.o dht_utils.o
	${CC} ${CFLAGS} -o s_join s_join.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_handler: test_handler.c handler_wrapper.c dht.o dht_utils.o dht_client.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/PUT_HANDLER_NAME/test_handler/g' handler_wrapper.c > test_handler_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > test_handler_shepherd.c
	${CC} ${CFLAGS} -c test_handler_shepherd.c -o test_handler_shepherd.o
	${CC} ${CFLAGS} -c test_handler.c -o test_handler.o
	${CC} ${CFLAGS} -o test_handler test_handler_wrapped.c test_handler.o test_handler_shepherd.o dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_publish: test_publish.c dht.o dht_utils.o dht_client.o
	${CC} ${CFLAGS} -o test_publish test_publish.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_subscribe: test_subscribe.c dht.o dht_utils.o dht_client.o
	${CC} ${CFLAGS} -o test_subscribe test_subscribe.c dht.o dht_utils.o dht_client.o ${RAFTOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o *_shepherd.c s_create s_join client_find_node client_node_info d_check_predecessor d_daemon d_fix_finger d_stablize h_find_result h_find_successor h_fix_finger_callback h_get_predecessor h_join_callback h_notify_callback h_notify h_register_topic h_stablize_callback h_subscribe h_trigger test_handler test_publish test_subscribe

