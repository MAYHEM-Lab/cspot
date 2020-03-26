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

all: raft.o count_vote create_woofs review_request start_server term_chair timeout_checker 
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp count_vote ./cspot;
	cp create_woofs ./cspot;
	cp review_request ./cspot;
	cp start_server ./cspot;
	cp term_chair ./cspot;
	cp timeout_checker ./cspot;

raft.o: raft.c raft.h
	${CC} ${CFLAGS} -c raft.c

count_vote: count_vote.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/count_vote/g' ${SHEP_SRC} > count_vote_shepherd.c
	${CC} ${CFLAGS} -c count_vote_shepherd.c -o count_vote_shepherd.o
	${CC} ${CFLAGS} -o count_vote count_vote.c count_vote_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

create_woofs: create_woofs.c raft.h
	${CC} ${CFLAGS} -o create_woofs create_woofs.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp create_woofs ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

review_request: review_request.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/review_request/g' ${SHEP_SRC} > review_request_shepherd.c
	${CC} ${CFLAGS} -c review_request_shepherd.c -o review_request_shepherd.o
	${CC} ${CFLAGS} -o review_request review_request.c review_request_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

start_server: start_server.c raft.h
	${CC} ${CFLAGS} -o start_server start_server.c raft.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp start_server ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

term_chair: term_chair.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/term_chair/g' ${SHEP_SRC} > term_chair_shepherd.c
	${CC} ${CFLAGS} -c term_chair_shepherd.c -o term_chair_shepherd.o
	${CC} ${CFLAGS} -o term_chair term_chair.c term_chair_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

timeout_checker: timeout_checker.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/timeout_checker/g' ${SHEP_SRC} > timeout_checker_shepherd.c
	${CC} ${CFLAGS} -c timeout_checker_shepherd.c -o timeout_checker_shepherd.o
	${CC} ${CFLAGS} -o timeout_checker timeout_checker.c timeout_checker_shepherd.o raft.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot raft.o count_vote create_woofs review_request start_server term_chair timeout_checker 

