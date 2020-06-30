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

DHTOBJ=dht.o dht_utils.o dht_client.o
RAFTDIR=${WOOFC}/apps/raft
RAFTOBJ=${RAFTDIR}/raft.o ${RAFTDIR}/raft_client.o
MONITORINC=${WOOFC}/apps/monitor/
MONITOROBJ=${WOOFC}/apps/monitor/monitor.o

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${RAFTDIR} -I${MONITORINC} ${MAKEOPT}

all: handlers copy clients test
	@echo MAKEOPT=${MAKEOPT}
	@echo CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${RAFTDIR} ${MAKEOPT}

copy: handlers clients test
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp d_check_predecessor ./cspot;
	cp d_daemon ./cspot;
	cp d_fix_finger ./cspot;
	cp d_replicate_state ./cspot;
	cp d_stabilize ./cspot;
	cp h_find_result ./cspot;
	cp h_find_successor ./cspot;
	cp h_fix_finger_callback ./cspot;
	cp h_get_predecessor ./cspot;
	cp h_invalidate_fingers ./cspot;
	cp h_join_callback ./cspot;
	cp h_notify_callback ./cspot;
	cp h_notify ./cspot;
	cp h_register_topic ./cspot;
	cp h_shift_successor ./cspot;
	cp h_stabilize_callback ./cspot;
	cp h_subscribe ./cspot;
	cp h_trigger ./cspot;
	cp s_create_cluster ./cspot;
	cp s_create_woofs ./cspot;
	cp s_join_cluster ./cspot;
	cp s_start_daemon ./cspot;
	cp r_set_finger ./cspot;
	cp r_set_node ./cspot;
	cp r_set_predecessor ./cspot;
	cp r_set_successor ./cspot;
	cp r_try_replicas ./cspot;
	cp client_find_node ./cspot;
	cp client_init_topic ./cspot;
	cp client_node_info ./cspot;
	cp client_publish ./cspot;
	cp client_subscribe ./cspot;
	cp test_dht_handler ./cspot;
	cp test_publish ./cspot;
	cp test_subscribe ./cspot;
	cp ../monitor/monitor_invoker ./cspot
	
handlers: d_check_predecessor d_daemon d_fix_finger d_replicate_state d_stabilize \
	h_find_result h_find_successor h_fix_finger_callback h_get_predecessor h_invalidate_fingers h_join_callback h_notify_callback \
	h_notify h_register_topic h_shift_successor h_stabilize_callback h_subscribe h_trigger \
	r_set_finger r_set_node r_set_predecessor r_set_successor r_try_replicas

clients: s_create_cluster s_create_woofs s_join_cluster s_start_daemon \
	client_find_node client_init_topic client_node_info client_publish client_subscribe

test: test_dht_handler test_publish test_subscribe

client_find_node: client_find_node.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_find_node client_find_node.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_init_topic: client_init_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_init_topic client_init_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_node_info: client_node_info.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_node_info client_node_info.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_publish: client_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_publish client_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_subscribe: client_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_subscribe client_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

dht_client.o: dht_client.c dht_client.h
	$(CC) $(CFLAGS) -c dht_client.c
	
dht_utils.o: dht_utils.c dht_utils.h
	${CC} ${CFLAGS} -c dht_utils.c

dht.o: dht.c dht.h
	${CC} ${CFLAGS} -c dht.c

d_check_predecessor: d_check_predecessor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_check_predecessor/g' ${SHEP_SRC} > d_check_predecessor_shepherd.c
	${CC} ${CFLAGS} -c d_check_predecessor_shepherd.c -o d_check_predecessor_shepherd.o
	${CC} ${CFLAGS} -o d_check_predecessor d_check_predecessor.c d_check_predecessor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_daemon: d_daemon.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_daemon/g' ${SHEP_SRC} > d_daemon_shepherd.c
	${CC} ${CFLAGS} -c d_daemon_shepherd.c -o d_daemon_shepherd.o
	${CC} ${CFLAGS} -o d_daemon d_daemon.c d_daemon_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_fix_finger: d_fix_finger.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_fix_finger/g' ${SHEP_SRC} > d_fix_finger_shepherd.c
	${CC} ${CFLAGS} -c d_fix_finger_shepherd.c -o d_fix_finger_shepherd.o
	${CC} ${CFLAGS} -o d_fix_finger d_fix_finger.c d_fix_finger_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_replicate_state: d_replicate_state.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_replicate_state/g' ${SHEP_SRC} > d_replicate_state_shepherd.c
	${CC} ${CFLAGS} -c d_replicate_state_shepherd.c -o d_replicate_state_shepherd.o
	${CC} ${CFLAGS} -o d_replicate_state d_replicate_state.c d_replicate_state_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

d_stabilize: d_stabilize.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_stabilize/g' ${SHEP_SRC} > d_stabilize_shepherd.c
	${CC} ${CFLAGS} -c d_stabilize_shepherd.c -o d_stabilize_shepherd.o
	${CC} ${CFLAGS} -o d_stabilize d_stabilize.c d_stabilize_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_find_result: h_find_result.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_find_result/g' ${SHEP_SRC} > h_find_result_shepherd.c
	${CC} ${CFLAGS} -c h_find_result_shepherd.c -o h_find_result_shepherd.o
	${CC} ${CFLAGS} -o h_find_result h_find_result.c h_find_result_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_find_successor: h_find_successor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_find_successor/g' ${SHEP_SRC} > h_find_successor_shepherd.c
	${CC} ${CFLAGS} -c h_find_successor_shepherd.c -o h_find_successor_shepherd.o
	${CC} ${CFLAGS} -o h_find_successor h_find_successor.c h_find_successor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_fix_finger_callback: h_fix_finger_callback.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_fix_finger_callback/g' ${SHEP_SRC} > h_fix_finger_callback_shepherd.c
	${CC} ${CFLAGS} -c h_fix_finger_callback_shepherd.c -o h_fix_finger_callback_shepherd.o
	${CC} ${CFLAGS} -o h_fix_finger_callback h_fix_finger_callback.c h_fix_finger_callback_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_get_predecessor: h_get_predecessor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_get_predecessor/g' ${SHEP_SRC} > h_get_predecessor_shepherd.c
	${CC} ${CFLAGS} -c h_get_predecessor_shepherd.c -o h_get_predecessor_shepherd.o
	${CC} ${CFLAGS} -o h_get_predecessor h_get_predecessor.c h_get_predecessor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_invalidate_fingers: h_invalidate_fingers.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_invalidate_fingers/g' ${SHEP_SRC} > h_invalidate_fingers_shepherd.c
	${CC} ${CFLAGS} -c h_invalidate_fingers_shepherd.c -o h_invalidate_fingers_shepherd.o
	${CC} ${CFLAGS} -o h_invalidate_fingers h_invalidate_fingers.c h_invalidate_fingers_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_join_callback: h_join_callback.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_join_callback/g' ${SHEP_SRC} > h_join_callback_shepherd.c
	${CC} ${CFLAGS} -c h_join_callback_shepherd.c -o h_join_callback_shepherd.o
	${CC} ${CFLAGS} -o h_join_callback h_join_callback.c h_join_callback_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_notify_callback: h_notify_callback.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_notify_callback/g' ${SHEP_SRC} > h_notify_callback_shepherd.c
	${CC} ${CFLAGS} -c h_notify_callback_shepherd.c -o h_notify_callback_shepherd.o
	${CC} ${CFLAGS} -o h_notify_callback h_notify_callback.c h_notify_callback_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_notify: h_notify.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_notify/g' ${SHEP_SRC} > h_notify_shepherd.c
	${CC} ${CFLAGS} -c h_notify_shepherd.c -o h_notify_shepherd.o
	${CC} ${CFLAGS} -o h_notify h_notify.c h_notify_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_register_topic: h_register_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_register_topic/g' ${SHEP_SRC} > h_register_topic_shepherd.c
	${CC} ${CFLAGS} -c h_register_topic_shepherd.c -o h_register_topic_shepherd.o
	${CC} ${CFLAGS} -o h_register_topic h_register_topic.c h_register_topic_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_shift_successor: h_shift_successor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_shift_successor/g' ${SHEP_SRC} > h_shift_successor_shepherd.c
	${CC} ${CFLAGS} -c h_shift_successor_shepherd.c -o h_shift_successor_shepherd.o
	${CC} ${CFLAGS} -o h_shift_successor h_shift_successor.c h_shift_successor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_stabilize_callback: h_stabilize_callback.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_stabilize_callback/g' ${SHEP_SRC} > h_stabilize_callback_shepherd.c
	${CC} ${CFLAGS} -c h_stabilize_callback_shepherd.c -o h_stabilize_callback_shepherd.o
	${CC} ${CFLAGS} -o h_stabilize_callback h_stabilize_callback.c h_stabilize_callback_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_subscribe: h_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_subscribe/g' ${SHEP_SRC} > h_subscribe_shepherd.c
	${CC} ${CFLAGS} -c h_subscribe_shepherd.c -o h_subscribe_shepherd.o
	${CC} ${CFLAGS} -o h_subscribe h_subscribe.c h_subscribe_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_trigger: h_trigger.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_trigger/g' ${SHEP_SRC} > h_trigger_shepherd.c
	${CC} ${CFLAGS} -c h_trigger_shepherd.c -o h_trigger_shepherd.o
	${CC} ${CFLAGS} -o h_trigger h_trigger.c h_trigger_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_set_finger: r_set_finger.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_set_finger/g' ${SHEP_SRC} > r_set_finger_shepherd.c
	${CC} ${CFLAGS} -c r_set_finger_shepherd.c -o r_set_finger_shepherd.o
	${CC} ${CFLAGS} -o r_set_finger r_set_finger.c r_set_finger_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_set_node: r_set_node.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_set_node/g' ${SHEP_SRC} > r_set_node_shepherd.c
	${CC} ${CFLAGS} -c r_set_node_shepherd.c -o r_set_node_shepherd.o
	${CC} ${CFLAGS} -o r_set_node r_set_node.c r_set_node_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_set_predecessor: r_set_predecessor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_set_predecessor/g' ${SHEP_SRC} > r_set_predecessor_shepherd.c
	${CC} ${CFLAGS} -c r_set_predecessor_shepherd.c -o r_set_predecessor_shepherd.o
	${CC} ${CFLAGS} -o r_set_predecessor r_set_predecessor.c r_set_predecessor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_set_successor: r_set_successor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_set_successor/g' ${SHEP_SRC} > r_set_successor_shepherd.c
	${CC} ${CFLAGS} -c r_set_successor_shepherd.c -o r_set_successor_shepherd.o
	${CC} ${CFLAGS} -o r_set_successor r_set_successor.c r_set_successor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_try_replicas: r_try_replicas.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_try_replicas/g' ${SHEP_SRC} > r_try_replicas_shepherd.c
	${CC} ${CFLAGS} -c r_try_replicas_shepherd.c -o r_try_replicas_shepherd.o
	${CC} ${CFLAGS} -o r_try_replicas r_try_replicas.c r_try_replicas_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_create_cluster: s_create_cluster.c dht.o dht_utils.o
	${CC} ${CFLAGS} -o s_create_cluster s_create_cluster.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_create_woofs: s_create_woofs.c dht.o dht_utils.o
	${CC} ${CFLAGS} -o s_create_woofs s_create_woofs.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_join_cluster: s_join_cluster.c dht.o dht_utils.o
	${CC} ${CFLAGS} -o s_join_cluster s_join_cluster.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_start_daemon: s_start_daemon.c dht.o dht_utils.o
	${CC} ${CFLAGS} -o s_start_daemon s_start_daemon.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_dht_handler: test_dht_handler.c handler_wrapper.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/PUT_HANDLER_NAME/test_dht_handler/g' handler_wrapper.c > test_dht_handler_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > test_dht_handler_shepherd.c
	${CC} ${CFLAGS} -c test_dht_handler_shepherd.c -o test_dht_handler_shepherd.o
	${CC} ${CFLAGS} -c test_dht_handler.c -o test_dht_handler.o
	${CC} ${CFLAGS} -o test_dht_handler test_dht_handler_wrapped.c test_dht_handler.o test_dht_handler_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_publish: test_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_publish test_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_subscribe: test_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_subscribe test_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o *_shepherd.c
	rm -f s_create_cluster s_create_woofs s_join_cluster s_start_daemon
	rm -f client_find_node client_node_info
	rm -f d_check_predecessor d_daemon d_fix_finger d_replicate_state d_stabilize
	rm -f h_find_result h_find_successor h_fix_finger_callback h_get_predecessor h_invalidate_fingers
	rm -f h_join_callback h_notify_callback h_notify h_register_topic h_shift_successor h_stabilize_callback h_subscribe h_trigger
	rm -f r_set_finger r_set_node r_set_predecessor r_set_successor r_try_replicas
	rm -f test_dht_handler test_publish test_subscribe

