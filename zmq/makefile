CC=gcc
LIBS=-lpthread -lczmq
CFLAGS=-g

all: woofc-msg-test-server woofc-msg-test-client

woofc-msg-test-server: woofc-msg.c
	${CC} ${CFLAGS} -DSERVER -o woofc-msg-test-server woofc-msg.c ${LIBS}

woofc-msg-test-client: woofc-msg.c
	${CC} ${CFLAGS} -DCLIENT -o woofc-msg-test-client woofc-msg.c ${LIBS}
