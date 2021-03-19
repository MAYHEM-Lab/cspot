CC=gcc
DEP=../../..
WOOFC=../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq -lpaho-mqtt3a
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o ${WOOFC}/repair.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h ${WOOFC}/repair.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h 
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h 
WHOBJ=${WOOFC}/woofc-host.o 
SHEP_SRC=${WOOFC}/woofc-shepherd.c

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: mqttc_create mqttc_pub mqttc_sub mqttc_test mqttc_woof_sub

mqttc_create: mqttc_create.c mqttc_test.h
	${CC} ${CFLAGS} -o mqttc_create mqttc_create.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp mqttc_create ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

mqttc_pub: mqttc_pub.c mqttc_test.h
	${CC} ${CFLAGS} -o mqttc_pub mqttc_pub.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp mqttc_pub ./cspot;

mqttc_sub: mqttc_sub.c mqttc_test.h
	${CC} ${CFLAGS} -o mqttc_sub mqttc_sub.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp mqttc_sub ./cspot;

mqttc_test: mqttc_test.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/mqttc_test/g' ${SHEP_SRC} > mqttc_test_shepherd.c
	${CC} ${CFLAGS} -c mqttc_test_shepherd.c -o mqttc_test_shepherd.o
	${CC} ${CFLAGS} -o mqttc_test mqttc_test.c mqttc_test_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp mqttc_test ./cspot;

mqttc_woof_sub: mqttc_woof_sub.c mqttc_test.h
	${CC} ${CFLAGS} -o mqttc_woof_sub mqttc_woof_sub.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp mqttc_woof_sub ./cspot;

clean:
	rm -f mqttc_create mqttc_pub mqttc_sub mqttc_test mqttc_woof_sub
