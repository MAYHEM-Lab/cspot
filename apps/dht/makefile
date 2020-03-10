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

all: create join find find_result find_successor join_callback stablize fix_fingers check_predecessor get_predecessor notify init_topic subscribe trigger stablize_callback fix_fingers_callback dht_daemon test sub_test test2
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp create ./cspot;
	cp join ./cspot;
	cp find ./cspot;
	cp find_result ./cspot;
	cp find_successor ./cspot;
	cp join_callback ./cspot;
	cp stablize ./cspot;
	cp fix_fingers ./cspot;
	cp check_predecessor ./cspot;
	cp get_predecessor ./cspot;
	cp notify ./cspot;
	cp init_topic ./cspot;
	cp subscribe ./cspot;
	cp trigger ./cspot;
	cp stablize_callback ./cspot;
	cp fix_fingers_callback ./cspot;
	cp dht_daemon ./cspot;
	cp test ./cspot;
	cp sub_test ./cspot;
	cp test2 ./cspot;

create: create.c dht.o
	${CC} ${CFLAGS} -o create create.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

join: join.c dht.o
	${CC} ${CFLAGS} -o join join.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

find: find.c dht.o
	${CC} ${CFLAGS} -o find find.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

stablize: stablize.c dht.o
	${CC} ${CFLAGS} -o stablize stablize.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

fix_fingers: fix_fingers.c dht.o
	${CC} ${CFLAGS} -o fix_fingers fix_fingers.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

check_predecessor: check_predecessor.c dht.o
	${CC} ${CFLAGS} -o check_predecessor check_predecessor.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

dht_daemon: dht_daemon.c dht.o
	${CC} ${CFLAGS} -o dht_daemon dht_daemon.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

dht.o: dht.c dht.h
	${CC} ${CFLAGS} -c dht.c

find_result: find_result.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/find_result/g' ${SHEP_SRC} > find_result_shepherd.c
	${CC} ${CFLAGS} -c find_result_shepherd.c -o find_result_shepherd.o
	${CC} ${CFLAGS} -o find_result find_result.c find_result_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

find_successor: find_successor.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/find_successor/g' ${SHEP_SRC} > find_successor_shepherd.c
	${CC} ${CFLAGS} -c find_successor_shepherd.c -o find_successor_shepherd.o
	${CC} ${CFLAGS} -o find_successor find_successor.c find_successor_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

join_callback: join_callback.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/join_callback/g' ${SHEP_SRC} > join_callback_shepherd.c
	${CC} ${CFLAGS} -c join_callback_shepherd.c -o join_callback_shepherd.o
	${CC} ${CFLAGS} -o join_callback join_callback.c join_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

get_predecessor: get_predecessor.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/get_predecessor/g' ${SHEP_SRC} > get_predecessor_shepherd.c
	${CC} ${CFLAGS} -c get_predecessor_shepherd.c -o get_predecessor_shepherd.o
	${CC} ${CFLAGS} -o get_predecessor get_predecessor.c get_predecessor_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

notify: notify.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/notify/g' ${SHEP_SRC} > notify_shepherd.c
	${CC} ${CFLAGS} -c notify_shepherd.c -o notify_shepherd.o
	${CC} ${CFLAGS} -o notify notify.c notify_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

init_topic: init_topic.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/init_topic/g' ${SHEP_SRC} > init_topic_shepherd.c
	${CC} ${CFLAGS} -c init_topic_shepherd.c -o init_topic_shepherd.o
	${CC} ${CFLAGS} -o init_topic init_topic.c init_topic_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

subscribe: subscribe.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/subscribe/g' ${SHEP_SRC} > subscribe_shepherd.c
	${CC} ${CFLAGS} -c subscribe_shepherd.c -o subscribe_shepherd.o
	${CC} ${CFLAGS} -o subscribe subscribe.c subscribe_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

trigger: trigger.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/trigger/g' ${SHEP_SRC} > trigger_shepherd.c
	${CC} ${CFLAGS} -c trigger_shepherd.c -o trigger_shepherd.o
	${CC} ${CFLAGS} -o trigger trigger.c trigger_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

stablize_callback: stablize_callback.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/stablize_callback/g' ${SHEP_SRC} > stablize_callback_shepherd.c
	${CC} ${CFLAGS} -c stablize_callback_shepherd.c -o stablize_callback_shepherd.o
	${CC} ${CFLAGS} -o stablize_callback stablize_callback.c stablize_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

fix_fingers_callback: fix_fingers_callback.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/fix_fingers_callback/g' ${SHEP_SRC} > fix_fingers_callback_shepherd.c
	${CC} ${CFLAGS} -c fix_fingers_callback_shepherd.c -o fix_fingers_callback_shepherd.o
	${CC} ${CFLAGS} -o fix_fingers_callback fix_fingers_callback.c fix_fingers_callback_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

sub_test: sub_test.c dht.o
	${CC} ${CFLAGS} -o sub_test sub_test.c dht.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test: test.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test/g' ${SHEP_SRC} > test_shepherd.c
	${CC} ${CFLAGS} -c test_shepherd.c -o test_shepherd.o
	${CC} ${CFLAGS} -o test test.c test_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test2: test2.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test2/g' ${SHEP_SRC} > test2_shepherd.c
	${CC} ${CFLAGS} -c test2_shepherd.c -o test2_shepherd.o
	${CC} ${CFLAGS} -o test2 test2.c test2_shepherd.o dht.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot create join find find_result find_successor join_callback stablize fix_fingers check_predecessor get_predecessor notify init_topic subscribe trigger stablize_callback fix_fingers_callback dht_daemon *.o

