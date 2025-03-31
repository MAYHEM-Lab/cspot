CC=gcc
CFLAGS=-g -Wformat-truncation=0 -I ../../include

RBINC=/home/ubuntu/src/cspot/deps/euca-cutils
RBSRC=/home/ubuntu/src/cspot/deps/euca-cutils/redblack.c

all: cmq-frame-selftest cmq-test-client cmq-test-server cmq-mqtt-test-client cmq-mqtt-test-server

cmq-frame.o: cmq-frame.h cmq-frame.c
	${CC} ${CFLAGS} -c cmq-frame.c

cmq-mqtt-xport.o: cmq-mqtt-xport.c cmq-mqtt-xport.h
	${CC} ${CFLAGS} -c cmq-mqtt-xport.c

cmq-frame-selftest: cmq-frame.c cmq-frame.h
	${CC} ${CFLAGS} -DSELFTEST -o cmq-frame-selftest cmq-frame.c 

cmq-test-client: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c cmq-mqtt-xport.o cmq-mqtt-xport.h
	${CC} ${CFLAGS} -DTESTCLIENT -o cmq-test-client cmq-pkt.c cmq-frame.o cmq-mqtt-xport.o ${RBSRC} -lpthread -lyaml

cmq-test-server: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c cmq-mqtt-xport.o cmq-mqtt-xport.h
	${CC} ${CFLAGS} -DTESTSERVER -o cmq-test-server cmq-pkt.c cmq-frame.o cmq-mqtt-xport.o ${RBSRC} -lpthread -lyaml

cmq-mqtt-test-client: cmq-frame.o cmq-frame.h cmq-mqtt-xport.c cmq-mqtt-xport.h
	${CC} ${CFLAGS} -DTESTCLIENT -I${RBINC} -o cmq-mqtt-test-client cmq-mqtt-xport.c ${RBSRC} cmq-frame.o -lpthread -lyaml

cmq-mqtt-test-server: cmq-frame.o cmq-frame.h cmq-mqtt-xport.h cmq-mqtt-xport.c
	${CC} ${CFLAGS} -DTESTSERVER -I${RBINC} -o cmq-mqtt-test-server cmq-mqtt-xport.c ${RBSRC} cmq-frame.o -lpthread -lyaml 

clean:
	rm *.o cmq-frame-selftest cmq-test-client cmq-test-server cmq-mqtt-test-server cmq-mqtt-test-client

