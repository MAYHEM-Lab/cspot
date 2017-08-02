CC=gcc
UINC=../../euca-cutils
MINC=../../mio
SINC=../
ULIB=../../euca-cutils/libutils.a
MLIB=../../mio/mio.o ../../mio/mymalloc.o
SLIB=../lsema.o
LIBS=../uriparser2/liburiparser2.a -lpthread -lm
LOBJ=../log.o ../host.o ../event.o
LINC=../log.h ../host.h ../event.h
WINC=../woofc.h ../woofc-access.h
WOBJ=../woofc.o ../woofc-access.o
WHINC=../woofc-host.h
WHOBJ=../woofc-host.o
TINC=../woofc-thread.h
TOBJ=../woofc-thread.o
SHEP_SRC=../woofc-shepherd.c

HAND1=woofc_obj1_handler_1
HAND2=woofc_obj2_handler_1
HAND3=woofc_obj2_handler_2
HAND4=woofc_obj2_handler_3

HERE=/root/src/cspot/woofc-tests

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: woofc-test1 ${HAND1} ${HAND2} ${HAND3} ${HAND4} woofc-obj1-test woofc-obj2-test woofc-obj2-test-2

woofc-test1: woofc-test1.c ${TINC} ${TOBJ}
	${CC} ${CFLAGS} -o woofc-test1 woofc-test1.c ${TOBJ} ${SLIB} ${MLIB} ${LIBS}

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot
	${CC} ${CFLAGS} -DUSE_NS="${HERE}" -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot-ns1; cp ${HAND1} ./cspot-ns1; cp ../woofc-container ./cspot-ns1; cp ../woofc-namespace-platform ./cspot-ns1

${HAND2}: ${HAND2}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND2}/g' ${SHEP_SRC} > ${HAND2}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND2}_shepherd.c -o ${HAND2}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND2} ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot
	${CC} ${CFLAGS} -DUSE_NS="${HERE}" -o ${HAND2} ${HAND2}.c ${HAND2}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot-ns1; cp ${HAND2} ./cspot-ns1; cp ../woofc-container ./cspot-ns1; cp ../woofc-namespace-platform ./cspot-ns1

${HAND3}: ${HAND3}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND3}/g' ${SHEP_SRC} > ${HAND3}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND3}_shepherd.c -o ${HAND3}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND3} ${HAND3}.c ${HAND3}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND3} ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot
	${CC} ${CFLAGS} -DUSE_NS="${HERE}" -o ${HAND3} ${HAND3}.c ${HAND3}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot-ns2; cp ${HAND3} ./cspot-ns2; cp ../woofc-container ./cspot-ns2; cp ../woofc-namespace-platform ./cspot-ns2

${HAND4}: ${HAND4}.c ${SHEP_SRC} ${WINC} ${LINC} ${LOBJ} ${WOBJ} ${SLIB} ${SINC}
	sed 's/WOOF_HANDLER_NAME/${HAND4}/g' ${SHEP_SRC} > ${HAND4}_shepherd.c
	${CC} ${CFLAGS} -c ${HAND4}_shepherd.c -o ${HAND4}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND4} ${HAND4}.c ${HAND4}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND4} ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot
	${CC} ${CFLAGS} -DUSE_NS="${HERE}" -o ${HAND4} ${HAND4}.c ${HAND4}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot-ns3; cp ${HAND4} ./cspot-ns3; cp ../woofc-container ./cspot-ns3; cp ../woofc-namespace-platform ./cspot-ns3

woofc-obj1-test: woofc-obj1-test.c ${WINC} ${WHINC} ${LINC} ${LOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -o woofc-obj1-test woofc-obj1-test.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp woofc-obj1-test ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot

woofc-obj2-test: woofc-obj2-test.c ${WINC} ${WHINC} ${LINC} ${LOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -o woofc-obj2-test woofc-obj2-test.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp woofc-obj2-test ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot

woofc-obj2-test-2: woofc-obj2-test-2.c ${WINC} ${WHINC} ${LINC} ${LOBJ} ${WOBJ} ${WHOBJ} ${SLIB} ${SINC}
	${CC} ${CFLAGS} -o woofc-obj2-test-2 woofc-obj2-test-2.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp woofc-obj2-test-2 ./cspot; cp ../woofc-container ./cspot; cp ../woofc-namespace-platform ./cspot

clean:
	rm -f woofc-test1 woofc-obj1-test woofc-obj2-test woofc-obj2-test-2 ${HAND1} ${HAND2} ${HAND3} ${HAND4}

