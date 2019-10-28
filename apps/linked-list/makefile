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

all: app workload-generator #repair_test replica_test print_ledger slave

repair_test: repair_test.c Helper.o LinkedList.o CheckPointer.o ledger.o
	${CC} ${CFLAGS} -o repair_test repair_test.c Helper.o LinkedList.o CheckPointer.o ledger.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp repair_test ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

replica_test: replica_test.c Helper.o LinkedList.o CheckPointer.o ledger.o
	${CC} ${CFLAGS} -o replica_test replica_test.c Helper.o LinkedList.o CheckPointer.o ledger.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp replica_test ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

print_ledger: print_ledger.c Helper.o LinkedList.o CheckPointer.o ledger.o
	${CC} ${CFLAGS} -o print_ledger print_ledger.c Helper.o LinkedList.o CheckPointer.o ledger.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp print_ledger ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

slave: slave.c Helper.o LinkedList.o CheckPointer.o ledger.o
	${CC} ${CFLAGS} -o slave slave.c Helper.o LinkedList.o CheckPointer.o ledger.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp slave ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

workload-generator: workload-generator.c
	${CC} ${CFLAGS} -o workload-generator workload-generator.c

app: app.c Helper.o LinkedList.o CheckPointer.o ledger.o
	${CC} ${CFLAGS} -o app app.c Helper.o LinkedList.o CheckPointer.o ledger.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp app ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	
CheckPointer.o: CheckPointer.c
	${CC} ${CFLAGS} -c CheckPointer.c

Helper.o: Helper.c
	${CC} ${CFLAGS} -c Helper.c

LinkedList.o: LinkedList.c
	${CC} ${CFLAGS} -c LinkedList.c

ledger.o: ledger.c ledger.h
	${CC} ${CFLAGS} -c ledger.c

clean:
	rm -f app workload-generator repair_test replica_test print_ledger *.o

