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

test: test_monitor test_handler_concurrent test_handler_monitored objs copy
	cp test_monitor ./cspot
	cp test_handler_concurrent ./cspot
	cp test_handler_monitored ./cspot

monitor.o: monitor.c monitor.h
	${CC} ${CFLAGS} -c monitor.c

monitor_invoker: monitor_invoker.c objs ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/monitor_invoker/g' ${SHEP_SRC} > monitor_invoker_shepherd.c
	${CC} ${CFLAGS} -c monitor_invoker_shepherd.c -o monitor_invoker_shepherd.o
	${CC} ${CFLAGS} -o monitor_invoker monitor_invoker.c monitor_invoker_shepherd.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_handler_concurrent: test_handler_concurrent.c objs ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test_handler_concurrent/g' ${SHEP_SRC} > test_handler_concurrent_shepherd.c
	${CC} ${CFLAGS} -c test_handler_concurrent_shepherd.c -o test_handler_concurrent_shepherd.o
	${CC} ${CFLAGS} -o test_handler_concurrent test_handler_concurrent.c test_handler_concurrent_shepherd.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_handler_monitored: test_handler_monitored.c objs ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/test_handler_monitored/g' ${SHEP_SRC} > test_handler_monitored_shepherd.c
	${CC} ${CFLAGS} -c test_handler_monitored_shepherd.c -o test_handler_monitored_shepherd.o
	${CC} ${CFLAGS} -o test_handler_monitored test_handler_monitored.c test_handler_monitored_shepherd.o monitor.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

test_monitor: test_monitor.c test_monitor.h objs
	${CC} ${CFLAGS} -o test_monitor test_monitor.c monitor.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}

clean:
	rm -rf ./cspot *.o test_handler_concurrent test_handler_monitored test_monitor monitor_invoker