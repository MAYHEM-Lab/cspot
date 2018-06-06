CC=gcc
DEP=../../..
WOOFC=../..
URIDIR=../../uriparser2
UINC=${DEP}/euca-cutils
MINC=${DEP}/mio
SINC=${WOOFC}
LINC=${WOOFC}/log.h ${WOOFC}/host.h ${WOOFC}/event.h
WINC=${WOOFC}/woofc.h ${WOOFC}/woofc-access.h ${WOOFC}/woofc-cache.h
WHINC=${WOOFC}/woofc-host.h 
SHEP_SRC=${WOOFC}/lambda-shepherd.c
LIBS=liburiparser2.a -lpthread -lm -lczmq
WOBJ=woofc.o woofc-access.o woofc-cache.o
SLIB=lsema.o
WHOBJ=woofc-host.o 
LOBJ=log.o host.o event.o
MLIB=mio.o mymalloc.o
ULIB=libutils.a
HAND1=hw

CFLAGS=-g -I${UINC} -I${MINC} -I${SINC}

all: start_lambda ${HAND1} lib${HAND1}.so

start_lambda: start_lambda.c hw.h liburiparser2.a ${WOBJ} ${SLIB} ${WHOBJ} ${LOBJ} ${MLIB} ${ULIB}
	${CC} ${CFLAGS} -o start_lambda start_lambda.c boto3.c ${WOBJ} ${SLIB} ${WHOBJ} ${LOBJ} ${MLIB} ${ULIB} ${LIBS} -I /usr/include/python2.7 -L /usr/lib/python2.7 -lpython2.7

${HAND1}: ${HAND1}.c ${SHEP_SRC} ${WINC} ${LINC} ${SINC} liburiparser2.a ${WOBJ} ${SLIB} ${LOBJ} ${MLID} ${ULIB}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -fPIC -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -o ${HAND1} ${HAND1}.c ${HAND1}_shepherd.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${HAND1} ./cspot; cp ../../woofc-container ./cspot; cp ../../woofc-namespace-platform ./cspot

lib${HAND1}.so: ${HAND1}.c libutils.a liburiparser2.a ${WOJB} ${SLIB} ${WHOBJ} ${LOBJ} ${MLIB} ${ULIB}
	sed 's/WOOF_HANDLER_NAME/${HAND1}/g' ${SHEP_SRC} > ${HAND1}_shepherd.c
	${CC} ${CFLAGS} -fPIC -c ${HAND1}_shepherd.c -o ${HAND1}_shepherd.o
	${CC} ${CFLAGS} -fPIC -c ${HAND1}.c -o ${HAND1}.o
	${CC} ${CFLAGS} -shared -o lib${HAND1}.so *.o libutils.a ${LIBS}

woofc.o: ${WOOFC}/woofc.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/woofc.c -o woofc.o

woofc-access.o: ${WOOFC}/woofc-access.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/woofc-access.c -o woofc-access.o

woofc-cache.o: ${WOOFC}/woofc-cache.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/woofc-cache.c -o woofc-cache.o

lsema.o: ${WOOFC}/lsema.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/lsema.c -o lsema.o

woofc-host.o: ${WOOFC}/woofc-host.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/woofc-host.c -o woofc-host.o

log.o: ${WOOFC}/log.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/log.c -o log.o

host.o: ${WOOFC}/host.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/host.c -o host.o

event.o: ${WOOFC}/event.c
	${CC} ${CFLAGS} -fPIC -c ${WOOFC}/event.c -o event.o

mio.o: ${DEP}/mio/mio.c
	${CC} ${CFLAGS} -fPIC -c ${DEP}/mio/mio.c -o mio.o

mymalloc.o: ${DEP}/mio/mymalloc.c
	${CC} ${CFLAGS} -fPIC -c ${DEP}/mio/mymalloc.c -o mymalloc.o

libutils.a: ${DEP}/euca-cutils/dlist.c ${DEP}/euca-cutils/redblack.c ${DEP}/euca-cutils/textlist.c ${DEP}/euca-cutils/simple_input.c ${DEP}/euca-cutils/convert_time.c
	${CC} ${CFLAGS} -fPIC -c ${DEP}/euca-cutils/dlist.c -o dlist.o
	${CC} ${CFLAGS} -fPIC -c ${DEP}/euca-cutils/redblack.c -o redblack.o
	${CC} ${CFLAGS} -fPIC -c ${DEP}/euca-cutils/textlist.c -o textlist.o
	${CC} ${CFLAGS} -fPIC -c ${DEP}/euca-cutils/simple_input.c -o simple_input.o
	${CC} ${CFLAGS} -fPIC -c ${DEP}/euca-cutils/convert_time.c -o convert_time.o
	ar -cr libutils.a textlist.o dlist.o redblack.o simple_input.o convert_time.o

liburiparser2.a: ${URIDIR}/uriparser2.o ${URIDIR}/uriparser/UriParse.o ${URIDIR}/uriparser/UriParseBase.o ${URIDIR}/uriparser/UriCommon.o ${URIDIR}/uriparser/UriIp4Base.o ${URIDIR}/uriparser/UriIp4.o
	ar cr liburiparser2.a ${URIDIR}/uriparser2.o ${URIDIR}/uriparser/UriParse.o ${URIDIR}/uriparser/UriParseBase.o ${URIDIR}/uriparser/UriCommon.o ${URIDIR}/uriparser/UriIp4Base.o ${URIDIR}/uriparser/UriIp4.o

clean:
	rm -f start_lambda ${HAND1} *.o *.so *.a *.pyc

