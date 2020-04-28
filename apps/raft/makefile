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

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${WOOFC}/apps/monitor

all: test handlers copy clients

monitor_app: ../monitor/monitor.c ../monitor/monitor.h ../monitor/monitor_invoker.c 
	cd ../monitor; make monitor.o; make monitor_invoker; cd ../raft;
	cp -p ../monitor/monitor.o ./
	cp -p ../monitor/monitor_invoker ./

copy: handlers
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp create_woofs ./cspot
	cp h_* ./cspot
	cp start_server ./cspot
	cp ../monitor/monitor_invoker ./cspot

test: client_test config_test raft.o client.o

clients: client_get client_observe client_put client_reconfig raft.o client.o

handlers: create_woofs start_server\
h_append_entries h_client_put h_config_change h_count_vote h_replicate_entries h_request_vote h_timeout_checker\

raft.o: raft.c raft.h
	${CC} ${CFLAGS} -c raft.c

client.o: client.c client.h
	${CC} ${CFLAGS} -c client.c

client_get: client_get.c client.o raft.o monitor.o
	${CC} ${CFLAGS} -o client_get client_get.c client.o raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_observe: client_observe.c client.o raft.o monitor.o
	${CC} ${CFLAGS} -o client_observe client_observe.c client.o raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_put: client_put.c client.o raft.o monitor.o
	${CC} ${CFLAGS} -o client_put client_put.c client.o raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_reconfig: client_reconfig.c client.o raft.o monitor.o
	${CC} ${CFLAGS} -o client_reconfig client_reconfig.c client.o raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_test: client_test.c client.o raft.o monitor.o
	${CC} ${CFLAGS} -o client_test client_test.c client.o raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

config_test: config_test.c raft.o monitor.o
	${CC} ${CFLAGS} -o config_test config_test.c raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_append_entries: h_append_entries.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_append_entries/g' ${SHEP_SRC} > h_append_entries_shepherd.c
	${CC} ${CFLAGS} -c h_append_entries_shepherd.c -o h_append_entries_shepherd.o
	${CC} ${CFLAGS} -o h_append_entries h_append_entries.c h_append_entries_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_client_put: h_client_put.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_client_put/g' ${SHEP_SRC} > h_client_put_shepherd.c
	${CC} ${CFLAGS} -c h_client_put_shepherd.c -o h_client_put_shepherd.o
	${CC} ${CFLAGS} -o h_client_put h_client_put.c h_client_put_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_config_change: h_config_change.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_config_change/g' ${SHEP_SRC} > h_config_change_shepherd.c
	${CC} ${CFLAGS} -c h_config_change_shepherd.c -o h_config_change_shepherd.o
	${CC} ${CFLAGS} -o h_config_change h_config_change.c h_config_change_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_count_vote: h_count_vote.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_count_vote/g' ${SHEP_SRC} > h_count_vote_shepherd.c
	${CC} ${CFLAGS} -c h_count_vote_shepherd.c -o h_count_vote_shepherd.o
	${CC} ${CFLAGS} -o h_count_vote h_count_vote.c h_count_vote_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_replicate_entries: h_replicate_entries.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_replicate_entries/g' ${SHEP_SRC} > h_replicate_entries_shepherd.c
	${CC} ${CFLAGS} -c h_replicate_entries_shepherd.c -o h_replicate_entries_shepherd.o
	${CC} ${CFLAGS} -o h_replicate_entries h_replicate_entries.c h_replicate_entries_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_request_vote: h_request_vote.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_request_vote/g' ${SHEP_SRC} > h_request_vote_shepherd.c
	${CC} ${CFLAGS} -c h_request_vote_shepherd.c -o h_request_vote_shepherd.o
	${CC} ${CFLAGS} -o h_request_vote h_request_vote.c h_request_vote_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_timeout_checker: h_timeout_checker.c raft.o monitor.o ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_timeout_checker/g' ${SHEP_SRC} > h_timeout_checker_shepherd.c
	${CC} ${CFLAGS} -c h_timeout_checker_shepherd.c -o h_timeout_checker_shepherd.o
	${CC} ${CFLAGS} -o h_timeout_checker h_timeout_checker.c h_timeout_checker_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

create_woofs: create_woofs.c raft.o monitor.o
	${CC} ${CFLAGS} -o create_woofs create_woofs.c raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

start_server: start_server.c raft.o monitor.o
	${CC} ${CFLAGS} -o start_server start_server.c raft.o monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot raft.o client.o client_test config_test client_get client_put client_reconfig h_count_vote create_woofs h_replicate_entries h_append_entries h_client_put h_config_change h_request_vote start_server term_chair h_timeout_checker 