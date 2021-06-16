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

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${RAFTDIR} -I${MONITORINC} ${MAKEOPT} -DPROFILING

all: handlers copy clients
	@echo MAKEOPT=${MAKEOPT}
	@echo CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${RAFTDIR} ${MAKEOPT}

copy: handlers clients
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot; cp ../../woofc-forker-helper ./cspot
	cp d_check_predecessor ./cspot;
	cp d_daemon ./cspot;
	cp d_fix_finger ./cspot;
	cp d_stabilize ./cspot;
	cp h_find_successor ./cspot;
	cp h_fix_finger_callback ./cspot;
	cp h_get_predecessor ./cspot;
	cp h_invalidate_cache ./cspot;
	cp h_invalidate_fingers ./cspot;
	cp h_join_callback ./cspot;
	cp h_notify_callback ./cspot;
	cp h_notify ./cspot;
	cp h_register_topic ./cspot;
	cp h_shift_successor ./cspot;
	cp h_stabilize_callback ./cspot;
	cp h_subscribe ./cspot;
	cp s_create_cluster ./cspot;
	cp s_create_woofs ./cspot;
	cp s_join_cluster ./cspot;
	cp r_create_index_map ./cspot;
	cp r_register_topic ./cspot;
	cp r_set_successor ./cspot;
	cp r_subscribe ./cspot;
	cp r_try_replicas ./cspot;
	cp server_publish_data ./cspot;
	cp server_publish_trigger ./cspot;
	cp client_check_leader ./cspot;
	cp client_find_node ./cspot;
	cp client_find_topic ./cspot;
	cp client_init_topic ./cspot;
	cp client_node_info ./cspot;
	cp client_publish ./cspot;
	cp client_subscribe ./cspot;
	cp ../monitor/monitor_invoker ./cspot
	
handlers: d_check_predecessor d_daemon d_fix_finger d_stabilize \
	h_find_successor h_fix_finger_callback h_get_predecessor h_invalidate_cache h_invalidate_fingers h_join_callback h_notify_callback \
	h_notify h_register_topic h_shift_successor h_stabilize_callback h_subscribe \
	r_create_index_map r_register_topic r_set_successor r_subscribe r_try_replicas \
	server_publish_data server_publish_trigger

clients: s_create_cluster s_create_woofs s_join_cluster \
	client_check_leader client_find_node client_find_topic client_init_topic client_node_info client_publish client_subscribe

test: test_dht_handler test_publish test_subscribe test_trigger_publish test_trigger_publish_multi test_latency_call test_latency_create test_latency_handler test_get
	mkdir -p cspot;
	cp test_dht_handler ./cspot;
	cp test_publish ./cspot;
	cp test_subscribe ./cspot;
	cp test_trigger_publish ./cspot;
	cp test_trigger_publish_multi ./cspot;
	cp test_latency_call ./cspot;
	cp test_latency_create ./cspot;
	cp test_latency_handler ./cspot;
	cp test_get ./cspot;

client_check_leader: client_check_leader.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_check_leader client_check_leader.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_find_node: client_find_node.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_find_node client_find_node.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_find_topic: client_find_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o client_find_topic client_find_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

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

d_stabilize: d_stabilize.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/d_stabilize/g' ${SHEP_SRC} > d_stabilize_shepherd.c
	${CC} ${CFLAGS} -c d_stabilize_shepherd.c -o d_stabilize_shepherd.o
	${CC} ${CFLAGS} -o d_stabilize d_stabilize.c d_stabilize_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

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

h_invalidate_cache: h_invalidate_cache.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_invalidate_cache/g' ${SHEP_SRC} > h_invalidate_cache_shepherd.c
	${CC} ${CFLAGS} -c h_invalidate_cache_shepherd.c -o h_invalidate_cache_shepherd.o
	${CC} ${CFLAGS} -o h_invalidate_cache h_invalidate_cache.c h_invalidate_cache_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

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

r_create_index_map: r_create_index_map.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_create_index_map/g' ${SHEP_SRC} > r_create_index_map_shepherd.c
	${CC} ${CFLAGS} -c r_create_index_map_shepherd.c -o r_create_index_map_shepherd.o
	${CC} ${CFLAGS} -o r_create_index_map r_create_index_map.c r_create_index_map_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_register_topic: r_register_topic.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_register_topic/g' ${SHEP_SRC} > r_register_topic_shepherd.c
	${CC} ${CFLAGS} -c r_register_topic_shepherd.c -o r_register_topic_shepherd.o
	${CC} ${CFLAGS} -o r_register_topic r_register_topic.c r_register_topic_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_set_successor: r_set_successor.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_set_successor/g' ${SHEP_SRC} > r_set_successor_shepherd.c
	${CC} ${CFLAGS} -c r_set_successor_shepherd.c -o r_set_successor_shepherd.o
	${CC} ${CFLAGS} -o r_set_successor r_set_successor.c r_set_successor_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

r_subscribe: r_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/r_subscribe/g' ${SHEP_SRC} > r_subscribe_shepherd.c
	${CC} ${CFLAGS} -c r_subscribe_shepherd.c -o r_subscribe_shepherd.o
	${CC} ${CFLAGS} -o r_subscribe r_subscribe.c r_subscribe_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

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

server_publish_data: server_publish_data.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/server_publish_data/g' ${SHEP_SRC} > server_publish_data_shepherd.c
	${CC} ${CFLAGS} -c server_publish_data_shepherd.c -o server_publish_data_shepherd.o
	${CC} ${CFLAGS} -o server_publish_data server_publish_data.c server_publish_data_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

server_publish_trigger: server_publish_trigger.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/server_publish_trigger/g' ${SHEP_SRC} > server_publish_trigger_shepherd.c
	${CC} ${CFLAGS} -c server_publish_trigger_shepherd.c -o server_publish_trigger_shepherd.o
	${CC} ${CFLAGS} -o server_publish_trigger server_publish_trigger.c server_publish_trigger_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_dht_handler: test_dht_handler.c handler_wrapper.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/PUT_HANDLER_NAME/test_dht_handler/g' handler_wrapper.c > test_dht_handler_wrapped.c
	sed 's/WOOF_HANDLER_NAME/handler_wrapper/g' ${SHEP_SRC} > test_dht_handler_shepherd.c
	${CC} ${CFLAGS} -c test_dht_handler_shepherd.c -o test_dht_handler_shepherd.o
	${CC} ${CFLAGS} -c test_dht_handler.c -o test_dht_handler.o
	${CC} ${CFLAGS} -o test_dht_handler test_dht_handler_wrapped.c test_dht_handler.o test_dht_handler_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_latency_call: test_latency_call.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_latency_call test_latency_call.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_latency_create: test_latency_create.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_latency_create test_latency_create.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_latency_handler: test_latency_handler.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test_latency_handler/g' ${SHEP_SRC} > test_latency_handler_shepherd.c
	${CC} ${CFLAGS} -c test_latency_handler_shepherd.c -o test_latency_handler_shepherd.o
	${CC} ${CFLAGS} -o test_latency_handler test_latency_handler.c test_latency_handler_shepherd.o ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_publish: test_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_publish test_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_subscribe: test_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_subscribe test_subscribe.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_trigger_publish: test_trigger_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_trigger_publish test_trigger_publish.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_trigger_publish_multi: test_trigger_publish_multi.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_trigger_publish_multi test_trigger_publish_multi.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_get: test_get.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ}
	${CC} ${CFLAGS} -o test_get test_get.c ${DHTOBJ} ${RAFTOBJ} ${MONITOROBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o *_shepherd.c
	rm -f s_create_cluster s_create_woofs s_join_cluster
	rm -f client_check_leader client_find_node client_find_topic client_init_topic client_node_info client_publish client_subscribe
	rm -f d_check_predecessor d_daemon d_fix_finger d_stabilize
	rm -f h_find_successor h_fix_finger_callback h_get_predecessor h_invalidate_cache h_invalidate_fingers
	rm -f h_join_callback h_notify_callback h_notify h_register_topic h_shift_successor h_stabilize_callback h_subscribe
	rm -f r_register_topic r_set_successor r_subscribe r_try_replicas
	rm -f server_publish_data server_publish_trigger
	rm -f test_dht_handler test_latency_call test_latency_create test_latency_handler test_publish test_subscribe test_trigger_publish test_trigger_publish_multi test_get

