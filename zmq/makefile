CC=gcc
LIBS=-lpthread -lczmq -lzmq
CFLAGS=-g

all: woofc-msg-test-server woofc-msg-test-client zproxy-thread-server zproxy-thread-client

woofc-msg-test-server: woofc-msg.c
	${CC} ${CFLAGS} -DSERVER -o woofc-msg-test-server woofc-msg.c ${LIBS}

woofc-msg-test-client: woofc-msg.c
	${CC} ${CFLAGS} -DCLIENT -o woofc-msg-test-client woofc-msg.c ${LIBS}

zproxy-thread-server: zproxy-thread.c
	${CC} ${CFLAGS} -DSERVER -o zproxy-thread-server zproxy-thread.c ${LIBS}

zproxy-thread-client: zproxy-thread.c
	${CC} ${CFLAGS} -DCLIENT -o zproxy-thread-client zproxy-thread.c ${LIBS}
