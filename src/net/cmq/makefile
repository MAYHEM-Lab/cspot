CC=gcc
CFLAGS=-g

all: cmq-frame-selftest cmq-test-client cmq-test-server

cmq-frame.o: cmq-frame.h cmq-frame.c
	${CC} ${CFLAGS} -c cmq-frame.c

cmq-frame-selftest: cmq-frame.c cmq-frame.h
	${CC} ${CFLAGS} -DSELFTEST -o cmq-frame-selftest cmq-frame.c 

cmq-test-client: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c
	${CC} ${CFLAGS} -DTESTCLIENT -o cmq-test-client cmq-pkt.c cmq-frame.o

cmq-test-server: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c
	${CC} ${CFLAGS} -DTESTSERVER -o cmq-test-server cmq-pkt.c cmq-frame.o

clean:
	rm *.o cmq-frame-selftest cmq-test-client cmq-test-server

