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

all: objs handlers test copy

copy:
	mkdir -p cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	cp monitor_invoker ./cspot

objs: monitor.o

handlers: monitor_invoker

test: monitor_test monitor_test_handler objs copy
	cp monitor_test ./cspot
	cp monitor_test_handler ./cspot

monitor.o: monitor.c monitor.h
	${CC} ${CFLAGS} -c monitor.c

monitor_invoker: monitor_invoker.c objs ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/monitor_invoker/g' ${SHEP_SRC} > monitor_invoker_shepherd.c
	${CC} ${CFLAGS} -c monitor_invoker_shepherd.c -o monitor_invoker_shepherd.o
	${CC} ${CFLAGS} -o monitor_invoker monitor_invoker.c monitor_invoker_shepherd.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

monitor_test: monitor_test.c monitor_test.h objs
	${CC} ${CFLAGS} -o monitor_test monitor_test.c monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

monitor_test_handler: monitor_test_handler.c objs ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/monitor_test_handler/g' ${SHEP_SRC} > monitor_test_handler_shepherd.c
	${CC} ${CFLAGS} -c monitor_test_handler_shepherd.c -o monitor_test_handler_shepherd.o
	${CC} ${CFLAGS} -o monitor_test_handler monitor_test_handler.c monitor_test_handler_shepherd.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o monitor_test_handler monitor_test monitor_invoker