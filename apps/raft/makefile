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

all: raft.o client.o append_entries_result client_get client_put client_test count_vote create_woofs replicate_entries review_append_entries review_client_put review_request_vote start_server term_chair timeout_checker 
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp append_entries_result ./cspot;
	cp count_vote ./cspot;
	cp create_woofs ./cspot;
	cp replicate_entries ./cspot;
	cp review_append_entries ./cspot;
	cp review_client_put ./cspot;
	cp review_request_vote ./cspot;
	cp start_server ./cspot;
	cp term_chair ./cspot;
	cp timeout_checker ./cspot;

raft.o: raft.c raft.h
	${CC} ${CFLAGS} -c raft.c

client.o: client.c client.h
	${CC} ${CFLAGS} -c client.c

append_entries_result: append_entries_result.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/append_entries_result/g' ${SHEP_SRC} > append_entries_result_shepherd.c
	${CC} ${CFLAGS} -c append_entries_result_shepherd.c -o append_entries_result_shepherd.o
	${CC} ${CFLAGS} -o append_entries_result append_entries_result.c append_entries_result_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_get: client_get.c client.o raft.o
	${CC} ${CFLAGS} -o client_get client_get.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_put: client_put.c client.o raft.o
	${CC} ${CFLAGS} -o client_put client_put.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

client_test: client_test.c client.o raft.o
	${CC} ${CFLAGS} -o client_test client_test.c client.o raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

count_vote: count_vote.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/count_vote/g' ${SHEP_SRC} > count_vote_shepherd.c
	${CC} ${CFLAGS} -c count_vote_shepherd.c -o count_vote_shepherd.o
	${CC} ${CFLAGS} -o count_vote count_vote.c count_vote_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

create_woofs: create_woofs.c raft.h
	${CC} ${CFLAGS} -o create_woofs create_woofs.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

replicate_entries: replicate_entries.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/replicate_entries/g' ${SHEP_SRC} > replicate_entries_shepherd.c
	${CC} ${CFLAGS} -c replicate_entries_shepherd.c -o replicate_entries_shepherd.o
	${CC} ${CFLAGS} -o replicate_entries replicate_entries.c replicate_entries_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

review_append_entries: review_append_entries.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/review_append_entries/g' ${SHEP_SRC} > review_append_entries_shepherd.c
	${CC} ${CFLAGS} -c review_append_entries_shepherd.c -o review_append_entries_shepherd.o
	${CC} ${CFLAGS} -o review_append_entries review_append_entries.c review_append_entries_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

review_client_put: review_client_put.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/review_client_put/g' ${SHEP_SRC} > review_client_put_shepherd.c
	${CC} ${CFLAGS} -c review_client_put_shepherd.c -o review_client_put_shepherd.o
	${CC} ${CFLAGS} -o review_client_put review_client_put.c review_client_put_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

review_request_vote: review_request_vote.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/review_request_vote/g' ${SHEP_SRC} > review_request_vote_shepherd.c
	${CC} ${CFLAGS} -c review_request_vote_shepherd.c -o review_request_vote_shepherd.o
	${CC} ${CFLAGS} -o review_request_vote review_request_vote.c review_request_vote_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

start_server: start_server.c raft.h
	${CC} ${CFLAGS} -o start_server start_server.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

term_chair: term_chair.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/term_chair/g' ${SHEP_SRC} > term_chair_shepherd.c
	${CC} ${CFLAGS} -c term_chair_shepherd.c -o term_chair_shepherd.o
	${CC} ${CFLAGS} -o term_chair term_chair.c term_chair_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

timeout_checker: timeout_checker.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/timeout_checker/g' ${SHEP_SRC} > timeout_checker_shepherd.c
	${CC} ${CFLAGS} -c timeout_checker_shepherd.c -o timeout_checker_shepherd.o
	${CC} ${CFLAGS} -o timeout_checker timeout_checker.c timeout_checker_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot raft.o client.o append_entries_result client_get client_put client_test count_vote create_woofs replicate_entries review_append_entries review_client_put review_request_vote start_server term_chair timeout_checker 

