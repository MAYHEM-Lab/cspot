CC=gcc
UINC=../euca-cutils
MINC=../mio
MINCS=../mio/mio.h
SINC=./
ULIB=../euca-cutils/libutils.a
MLIB=../mio/mio.o ../mio/mymalloc.o
SINCS=./lsema.h
SLIB=./lsema.o
LIBS=-lpthread -lm -lczmq
LOBJ=log.o host.o event.o
LINCS=log.h host.h event.h
WINCS=woofc.h woofc-access.h woofc-cache.h
WOBJ=woofc.o woofc-access.o woofc-cache.o
WHOBJ=woofc-host.h
WHOBJ=woofc-host.o
TINC=woofc-thread.h
TOBJ=woofc-thread.o
URIINC=./uriparser2
URILIB=./uriparser2/liburiparser2.a

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${URIINC} -DDEBUG
CXX_FLAGS=-g -std=c++11 -I${UINC} -I${MINC} -I${SINC} -I${URIINC} -DDEBUG

all: log-test log-test-thread woofc.o woofc-host.o woofc-shepherd.o woofc-container woofc-namespace-platform docker-image woofc-keygen

abd: log-test log-test-thread woofc.o woofc-host.o woofc-shepherd.o woofc-container woofc-namespace-platform

log-test: ${LOBJ} ${LINCS} log-test.c ${SLIB}
	${CC} ${CFLAGS} -o log-test log-test.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log-test-thread: ${LOBJ} ${LINCS} log-test-thread.c ${SLIB}
	${CC} ${CFLAGS} -o log-test-thread log-test-thread.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log.o: log.c log.h host.h event.h
	${CC} ${CFLAGS} -c log.c

woofc.o: woofc.c woofc.h
	${CC} ${CFLAGS} -c woofc.c

woofc-access.o: woofc-access.c woofc-access.h
	${CC} ${CFLAGS} -c woofc-access.c

woofc-cache.o: woofc-cache.c woofc-cache.h
	${CC} ${CFLAGS} -c woofc-cache.c

woofc-host.o: woofc-host.c woofc.h
	${CC} ${CFLAGS} -c woofc-host.c

woofc-shepherd.o: woofc-shepherd.c woofc.h
	${CC} ${CFLAGS} -c woofc-shepherd.c

woofc-thread.o: woofc-thread.c woofc-thread.h
	${CC} ${CFLAGS} -c woofc-thread.c

woofc-keygen: woofc-keygen.cpp
	${CXX} ${CXX_FLAGS} woofc-keygen.cpp -o woofc-keygen ${LIBS}

woofc-container: woofc-container.c ${LOBJ} ${WOBJ} ${SLIB} ${WINCS} ${SINCS} ${UINCS} ${LINCS} ${URILIB}
	${CC} ${CFLAGS} woofc-container.c -o woofc-container ${MLIB} ${LOBJ} ${WOBJ} ${SLIB} ${ULIB} ${URILIB} ${LIBS}

woofc-namespace-platform: woofc-host.c ${LOBJ} ${WOBJ} ${SLIB} ${WINC} ${SINCS} ${UINCS} ${LINCS} ${URILIB}
	${CC} ${CFLAGS} -DIS_PLATFORM woofc-host.c -o woofc-namespace-platform ${MLIB} ${LOBJ} ${WOBJ} ${SLIB} ${ULIB} ${URILIB} ${LIBS}

event.o: event.c event.h
	${CC} ${CFLAGS} -c event.c

host.o: host.c host.h event.h
	${CC} ${CFLAGS} -c host.c

sema.o: sema.c sema.h
	${CC} ${CFLAGS} -c sema.c

lsema.o: lsema.c lsema.h
	${CC} ${CFLAGS} -c lsema.c

${URILIB}:
	cd ./uriparser2;make

docker-image:
	cd Docker-build; docker build -t cspot-docker-centos7 .

force-docker:
	cd Docker-build; docker build --no-cache -t cspot-docker-centos7 .

clean:
	rm -f *.o log-test

