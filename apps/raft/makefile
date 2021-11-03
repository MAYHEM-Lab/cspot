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

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -DPROFILING

all: handlers copy clients

copy: handlers clients test
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot; cp ../../woofc-forker-helper ./cspot
	cp s_create_woofs ./cspot
	cp s_start_server ./cspot
	cp h_append_entries ./cspot
	cp h_client_put ./cspot
	cp h_config_change ./cspot
	cp h_count_vote ./cspot
	cp h_forward_put_result ./cspot
	cp h_replicate_entries ./cspot
	cp h_request_vote ./cspot
	cp h_timeout_checker ./cspot
	cp client_get ./cspot
	cp client_node_info ./cspot
	cp client_observe ./cspot
	cp client_put_result ./cspot
	cp client_put ./cspot
	cp client_reconfig ./cspot
	cp test_append_handler ./cspot
	cp test_client ./cspot
	cp test_config ./cspot
	cp test_raft_handler ./cspot
	cp test_throughput ./cspot
	cp test_throughput_handler ./cspot

test: test_append_handler test_client test_config test_raft_handler test_throughput test_throughput_handler raft.o raft_client.o raft_utils.o

clients: client_get client_node_info client_observe client_put_result client_put client_reconfig

handlers: s_create_woofs s_start_server\
h_append_entries h_client_put h_config_change h_count_vote h_forward_put_result h_replicate_entries h_request_vote h_timeout_checker\

raft.o: raft.c raft.h
	${CC} ${CFLAGS} -c raft.c

raft_client.o: raft_client.c raft_client.h
	${CC} ${CFLAGS} -c raft_client.c

raft_utils.o: raft_utils.c raft_utils.h
	${CC} ${CFLAGS} -c raft_utils.c

client_get: client_get.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o client_get client_get.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_node_info:  client_node_info.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o client_node_info client_node_info.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_observe: client_observe.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o client_observe client_observe.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_put_result: client_put_result.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o client_put_result client_put_result.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_put: client_put.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o client_put client_put.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_reconfig: client_reconfig.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o client_reconfig client_reconfig.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_client: test_client.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o test_client test_client.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_config: test_config.c raft.o raft_client.o raft_utils.o  
	${CC} ${CFLAGS} -o test_config test_config.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_append_entries: h_append_entries.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_append_entries/g' ${SHEP_SRC} > h_append_entries_shepherd.c
	${CC} ${CFLAGS} -c h_append_entries_shepherd.c -o h_append_entries_shepherd.o
	${CC} ${CFLAGS} -o h_append_entries h_append_entries.c h_append_entries_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_client_put: h_client_put.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_client_put/g' ${SHEP_SRC} > h_client_put_shepherd.c
	${CC} ${CFLAGS} -c h_client_put_shepherd.c -o h_client_put_shepherd.o
	${CC} ${CFLAGS} -o h_client_put h_client_put.c h_client_put_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_config_change: h_config_change.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_config_change/g' ${SHEP_SRC} > h_config_change_shepherd.c
	${CC} ${CFLAGS} -c h_config_change_shepherd.c -o h_config_change_shepherd.o
	${CC} ${CFLAGS} -o h_config_change h_config_change.c h_config_change_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_count_vote: h_count_vote.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_count_vote/g' ${SHEP_SRC} > h_count_vote_shepherd.c
	${CC} ${CFLAGS} -c h_count_vote_shepherd.c -o h_count_vote_shepherd.o
	${CC} ${CFLAGS} -o h_count_vote h_count_vote.c h_count_vote_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_forward_put_result: h_forward_put_result.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_forward_put_result/g' ${SHEP_SRC} > h_forward_put_result_shepherd.c
	${CC} ${CFLAGS} -c h_forward_put_result_shepherd.c -o h_forward_put_result_shepherd.o
	${CC} ${CFLAGS} -o h_forward_put_result h_forward_put_result.c h_forward_put_result_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_replicate_entries: h_replicate_entries.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_replicate_entries/g' ${SHEP_SRC} > h_replicate_entries_shepherd.c
	${CC} ${CFLAGS} -c h_replicate_entries_shepherd.c -o h_replicate_entries_shepherd.o
	${CC} ${CFLAGS} -o h_replicate_entries h_replicate_entries.c h_replicate_entries_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_request_vote: h_request_vote.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_request_vote/g' ${SHEP_SRC} > h_request_vote_shepherd.c
	${CC} ${CFLAGS} -c h_request_vote_shepherd.c -o h_request_vote_shepherd.o
	${CC} ${CFLAGS} -o h_request_vote h_request_vote.c h_request_vote_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_timeout_checker: h_timeout_checker.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_timeout_checker/g' ${SHEP_SRC} > h_timeout_checker_shepherd.c
	${CC} ${CFLAGS} -c h_timeout_checker_shepherd.c -o h_timeout_checker_shepherd.o
	${CC} ${CFLAGS} -o h_timeout_checker h_timeout_checker.c h_timeout_checker_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_create_woofs: s_create_woofs.c raft.o raft_client.o raft_utils.o
	${CC} ${CFLAGS} -o s_create_woofs s_create_woofs.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

s_start_server: s_start_server.c raft.o raft_client.o raft_utils.o
	${CC} ${CFLAGS} -o s_start_server s_start_server.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_append_handler: test_append_handler.c raft.o raft_client.o raft_utils.o
	${CC} ${CFLAGS} -o test_append_handler test_append_handler.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_raft_handler: test_raft_handler.c raft.o raft_client.o raft_utils.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test_raft_handler/g' ${SHEP_SRC} > test_raft_handler_shepherd.c
	${CC} ${CFLAGS} -c test_raft_handler_shepherd.c -o test_raft_handler_shepherd.o
	${CC} ${CFLAGS} -o test_raft_handler test_raft_handler.c test_raft_handler_shepherd.o raft.o raft_client.o raft_utils.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_throughput: test_throughput.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o test_throughput test_throughput.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_throughput_handler: test_throughput_handler.c raft.o raft_client.o raft_utils.o 
	${CC} ${CFLAGS} -o test_throughput_handler test_throughput_handler.c raft.o raft_client.o raft_utils.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o 
	rm -f test_append_handler test_client test_config test_raft_handler test_throughput test_throughput_handler
	rm -f client_get client_node_info client_observe client_put_result client_put client_reconfig
	rm -f h_append_entries h_client_put h_config_change h_count_vote h_forward_put_result h_replicate_entries h_request_vote h_timeout_checker
	rm -f s_create_woofs s_start_server 