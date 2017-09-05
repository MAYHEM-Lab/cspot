CC=gcc
DEP=../../..
WOOFC=../..
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
ULIB=${DEP}/euca-cutils/libutils.a
MLIB=${DEP}/mio/mio.o ${DEP}/mio/mymalloc.o
SLIB=${WOOFC}/lsema.o
LIBS=${WOOFC}/uriparser2/liburiparser2.a -lpthread -lm -lczmq -lpthread
LOBJ=${WOOFC}/log.o ${WOOFC}/host.o ${WOOFC}/event.o
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WOBJ=${WOOFC}/woofc.o ${WOOFC}/woofc-access.o ${WOOFC}/woofc-cache.o
WHINC=${WOOFC}/woofc-host.h
WHOBJ=${WOOFC}/woofc-host.o
TINC=${WOOFC}/woofc-thread.h
TOBJ=${WOOFC}/woofc-thread.o
SHEP_SRC=${WOOFC}/woofc-shepherd.c

HAND1=recv_start
HAND2=recv
HAND3=null_handler
HAND4=invoke_handler

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: bw-test put-test recv-init invoke-test ${HAND1} ${HAND2} ${HAND3} ${HAND4}

put-test: put-test.c put-test.h
	${CC} ${CFLAGS} -o put-test put-test.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp put-test ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp put-test ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

bw-test: bw-test.c put-test.h
	${CC} ${CFLAGS} -o bw-test bw-test.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp bw-test ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp bw-test ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

invoke-test: invoke-test.c
	${CC} ${CFLAGS} -o invoke-test invoke-test.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp invoke-test ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp invoke-test ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

recv-init: recv-init.c put-test.h
	${CC} ${CFLAGS} -o recv-init recv-init.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp recv-init ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp recv-init ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND1}: put-test.h ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND1} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND2}: put-test.h ${HAND2}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND2}/g' ${SHEP_SRC} > ${HAND2}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND2}_shepherd.c -o ${HAND2}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND2} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND2} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND3}: put-test.h ${HAND3}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND3}/g' ${SHEP_SRC} > ${HAND3}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND3}_shepherd.c -o ${HAND3}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND3} ${HAND3}.c ${HAND3}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND3} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND3} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

${HAND4}: ${HAND4}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND4}/g' ${SHEP_SRC} > ${HAND4}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND4}_shepherd.c -o ${HAND4}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND4} ${HAND4}.c ${HAND4}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND4} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot
	mkdir -p cspot-2; cp ${HAND4} ./cspot-2; cp ../../woofc-container ./cspot-2; cp ../../woofc-namespace-platform ./cspot-2

clean:
	rm -f recv-init put-test bw-test *.o ${HAND1} ${HAND2} ${HAND4}

