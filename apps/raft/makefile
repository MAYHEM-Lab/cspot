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

all: objs test copy clients

copy: h_append_entries_result h_count_vote create_woofs h_replicate_entries h_append_entries h_client_put h_reconfig h_request_vote start_server term_chair h_timeout_checker 
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp h_append_entries_result ./cspot;
	cp h_count_vote ./cspot;
	cp create_woofs ./cspot;
	cp h_replicate_entries ./cspot;
	cp h_append_entries ./cspot;
	cp h_client_put ./cspot;
	cp h_reconfig ./cspot;
	cp h_request_vote ./cspot;
	cp start_server ./cspot;
	cp term_chair ./cspot;
	cp h_timeout_checker ./cspot;

test: client_test config_test objs

clients: client_get client_put client_reconfig objs

objs: raft.o client.o monitor.o

raft.o: raft.c raft.h
	${CC} ${CFLAGS} -c raft.c

monitor.o: ../monitor.c ../monitor.h
	cd ../monitor; make; cd ../raft; cp ../monitor/monitor.o .

client.o: client.c client.h
	${CC} ${CFLAGS} -c client.c

h_append_entries_result: h_append_entries_result.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_append_entries_result/g' ${SHEP_SRC} > h_append_entries_result_shepherd.c
	${CC} ${CFLAGS} -c h_append_entries_result_shepherd.c -o h_append_entries_result_shepherd.o
	${CC} ${CFLAGS} -o h_append_entries_result h_append_entries_result.c h_append_entries_result_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_append_entries: h_append_entries.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_append_entries/g' ${SHEP_SRC} > h_append_entries_shepherd.c
	${CC} ${CFLAGS} -c h_append_entries_shepherd.c -o h_append_entries_shepherd.o
	${CC} ${CFLAGS} -o h_append_entries h_append_entries.c h_append_entries_shepherd.o raft.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_get: client_get.c objs
	${CC} ${CFLAGS} -o client_get client_get.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_put: client_put.c objs
	${CC} ${CFLAGS} -o client_put client_put.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_reconfig: client_reconfig.c objs
	${CC} ${CFLAGS} -o client_reconfig client_reconfig.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_test: client_test.c objs
	${CC} ${CFLAGS} -o client_test client_test.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

config_test: config_test.c raft.o 
	${CC} ${CFLAGS} -o config_test config_test.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_count_vote: h_count_vote.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_count_vote/g' ${SHEP_SRC} > h_count_vote_shepherd.c
	${CC} ${CFLAGS} -c h_count_vote_shepherd.c -o h_count_vote_shepherd.o
	${CC} ${CFLAGS} -o h_count_vote h_count_vote.c h_count_vote_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

create_woofs: create_woofs.c raft.h
	${CC} ${CFLAGS} -o create_woofs create_woofs.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_replicate_entries: h_replicate_entries.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_replicate_entries/g' ${SHEP_SRC} > h_replicate_entries_shepherd.c
	${CC} ${CFLAGS} -c h_replicate_entries_shepherd.c -o h_replicate_entries_shepherd.o
	${CC} ${CFLAGS} -o h_replicate_entries h_replicate_entries.c h_replicate_entries_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_request_vote: h_request_vote.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_request_vote/g' ${SHEP_SRC} > h_request_vote_shepherd.c
	${CC} ${CFLAGS} -c h_request_vote_shepherd.c -o h_request_vote_shepherd.o
	${CC} ${CFLAGS} -o h_request_vote h_request_vote.c h_request_vote_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_client_put: h_client_put.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_client_put/g' ${SHEP_SRC} > h_client_put_shepherd.c
	${CC} ${CFLAGS} -c h_client_put_shepherd.c -o h_client_put_shepherd.o
	${CC} ${CFLAGS} -o h_client_put h_client_put.c h_client_put_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_reconfig: h_reconfig.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_reconfig/g' ${SHEP_SRC} > h_reconfig_shepherd.c
	${CC} ${CFLAGS} -c h_reconfig_shepherd.c -o h_reconfig_shepherd.o
	${CC} ${CFLAGS} -o h_reconfig h_reconfig.c h_reconfig_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

start_server: start_server.c raft.h
	${CC} ${CFLAGS} -o start_server start_server.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

term_chair: term_chair.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/term_chair/g' ${SHEP_SRC} > term_chair_shepherd.c
	${CC} ${CFLAGS} -c term_chair_shepherd.c -o term_chair_shepherd.o
	${CC} ${CFLAGS} -o term_chair term_chair.c term_chair_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

h_timeout_checker: h_timeout_checker.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/h_timeout_checker/g' ${SHEP_SRC} > h_timeout_checker_shepherd.c
	${CC} ${CFLAGS} -c h_timeout_checker_shepherd.c -o h_timeout_checker_shepherd.o
	${CC} ${CFLAGS} -o h_timeout_checker h_timeout_checker.c h_timeout_checker_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot raft.o client.o client_test config_test h_append_entries_result client_get client_put client_reconfig h_count_vote create_woofs h_replicate_entries h_append_entries h_client_put h_reconfig h_request_vote start_server term_chair h_timeout_checker 