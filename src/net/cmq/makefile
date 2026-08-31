CC=gcc
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

CFLAGS += -g -I../../include -I../../../deps/euca-cutils
LIBS += -lpthread -lyaml

ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/opt/homebrew/opt/libyaml/include
    LDFLAGS += -L/opt/homebrew/opt/libyaml/lib
endif

RBINC=../../../deps/euca-cutils
RBSRC=../../../deps/euca-cutils/redblack.c

all: cmq-frame-selftest cmq-test-client cmq-test-server cmq-mqtt-test-client cmq-mqtt-test-server cmq-perf

cmq-frame.o: cmq-frame.h cmq-frame.c
	${CC} ${CFLAGS} -c cmq-frame.c

cmq-mqtt-xport.o: cmq-mqtt-xport.c cmq-mqtt-xport.h
	${CC} ${CFLAGS} -c cmq-mqtt-xport.c

cmq-cache.o: cmq-cache.c cmq-cache.h
	${CC} ${CFLAGS} -c cmq-cache.c

cmq-frame-selftest: cmq-frame.c cmq-frame.h
	${CC} ${CFLAGS} ${LDFLAGS} -DSELFTEST -o cmq-frame-selftest cmq-frame.c 

cmq-test-client: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c cmq-mqtt-xport.o cmq-mqtt-xport.h cmq-cache.o cmq-cache.h
	${CC} ${CFLAGS} ${LDFLAGS} -DTESTCLIENT -o cmq-test-client cmq-pkt.c cmq-frame.o cmq-mqtt-xport.o cmq-cache.o ${RBSRC} -lpthread -lyaml

cmq-test-server: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c cmq-mqtt-xport.o cmq-mqtt-xport.h cmq-cache.o cmq-cache.h
	${CC} ${CFLAGS} ${LDFLAGS} -DTESTSERVER -o cmq-test-server cmq-pkt.c cmq-frame.o cmq-mqtt-xport.o cmq-cache.o ${RBSRC} -lpthread -lyaml

cmq-mqtt-test-client: cmq-frame.o cmq-frame.h cmq-mqtt-xport.c cmq-mqtt-xport.h
	${CC} ${CFLAGS} ${LDFLAGS} -DTESTCLIENT -I${RBINC} -o cmq-mqtt-test-client cmq-mqtt-xport.c ${RBSRC} cmq-frame.o -lpthread -lyaml

cmq-mqtt-test-server: cmq-frame.o cmq-frame.h cmq-mqtt-xport.h cmq-mqtt-xport.c
	${CC} ${CFLAGS} ${LDFLAGS} -DTESTSERVER -I${RBINC} -o cmq-mqtt-test-server cmq-mqtt-xport.c ${RBSRC} cmq-frame.o -lpthread -lyaml 

cmq-perf: cmq-frame.o cmq-frame.h cmq-pkt.h cmq-pkt.c cmq-mqtt-xport.o cmq-mqtt-xport.h cmq-cache.o cmq-cache.h cmq-perf.c
	${CC} ${CFLAGS} ${LDFLAGS} -o cmq-perf cmq-perf.c cmq-pkt.c cmq-frame.o cmq-mqtt-xport.o cmq-cache.o ${RBSRC} -lpthread -lyaml

clean:
	rm *.o cmq-frame-selftest cmq-test-client cmq-test-server cmq-mqtt-test-server cmq-mqtt-test-client

