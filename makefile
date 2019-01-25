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
LOBJ=log.o host.o event.o repair.o
LINCS=log.h host.h event.h repair.h
WINCS=woofc.h woofc-access.h woofc-cache.h
WOBJ=woofc.o woofc-access.o woofc-cache.o
WHINC=woofc-host.h
WHOBJ=woofc-host.o
TINC=woofc-thread.h
TOBJ=woofc-thread.o
URIINC=./uriparser2
URILIB=./uriparser2/liburiparser2.a

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC} -I${URIINC}

all: log-test log-test-thread log-dump woof-repair repair.o woofc.o woofc-host.o woofc-shepherd.o woofc-container woofc-namespace-platform docker-image

abd: log-test log-test-thread log-dump woof-repair repair.o woofc.o woofc-host.o woofc-shepherd.o woofc-container woofc-namespace-platform

python: libwoof.so
	cp libwoof.so ./python-ext

log-test: ${LOBJ} ${LINCS} log-test.c ${SLIB}
	${CC} ${CFLAGS} -o log-test log-test.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log-test-thread: ${LOBJ} ${LINCS} log-test-thread.c ${SLIB}
	${CC} ${CFLAGS} -o log-test-thread log-test-thread.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

log-dump: ${LOBJ} ${LINCS} log-dump.c ${SLIB}
	${CC} ${CFLAGS} -o log-dump log-dump.c ${LOBJ} ${ULIB} ${MLIB} ${SLIB} ${LIBS}

woof-repair: woof-repair.c ${LOBJ} ${WOBJ} ${WHOBJ}
	${CC} ${CFLAGS} -o woof-repair woof-repair.c ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS} ${URILIB}

log.o: log.c log.h host.h event.h
	${CC} ${CFLAGS} -c log.c

repair.o: repair.c repair.h
	${CC} ${CFLAGS} -c repair.c

libwoof.so: woofc.c woofc.h woof-pyinit.c
	${CC} ${CFLAGS} -fPIC -c woof-pyinit.c -o pic-woof-pyinit.o
	${CC} ${CFLAGS} -fPIC -c woofc.c -o pic-woofc.o
	${CC} ${CFLAGS} -fPIC -c woofc-access.c -o pic-woofc-access.o
	${CC} ${CFLAGS} -fPIC -c woofc-cache.c -o pic-woofc-cache.o
	${CC} ${CFLAGS} -fPIC -c woofc-host.c -o pic-woofc-host.o
	${CC} ${CFLAGS} -fPIC -c log.c -o pic-log.o
	${CC} ${CFLAGS} -fPIC -c host.c -o pic-host.o
	${CC} ${CFLAGS} -fPIC -c event.c -o pic-event.o
	${CC} ${CFLAGS} -fPIC -c lsema.c -o pic-lsema.o
	${CC} ${CFLAGS} -fPIC -c ../mio/mio.c -o pic-mio.o
	${CC} ${CFLAGS} -fPIC -c ../mio/mymalloc.c -o pic-mymalloc.o
	cd ../euca-cutils; make; cd ../cspot;
	${CC} ${CFLAGS} -fPIC -c ../euca-cutils/dlist.c -o pic-dlist.o
	${CC} ${CFLAGS} -fPIC -c ../euca-cutils/redblack.c -o pic-redblack.o
	ar -cr pic-libutils.a ../euca-cutils/textlist.o pic-dlist.o pic-redblack.o ../euca-cutils/simple_input.o ../euca-cutils/convert_time.o
	${CC} ${CFLAGS} -shared -o libwoof.so pic-*.o ${LIBS} pic-libutils.a ${URILIB}

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
	rm -f *.o log-test *.so

