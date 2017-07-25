CC=gcc
UINC=../../../euca-cutils
UINCS=${UINC}/redblack.h
MINC=../../../mio
MINCS=${MINC}/mio.h
SINC=../..
SINCS=${SINC}/lsema.h
ULIB=../../../euca-cutils/libutils.a
MLIB=../../../mio/mio.o ../../../mio/mymalloc.o
SLIB=../../lsema.o
LIBS=-lpthread -lm
LOBJ=../../log.o ../../host.o ../../event.o
LINCS=../../log.h ../../host.h ../../event.h
WINC=../..
WINCS=${WINC}/woofc.h
WOBJ=../../woofc.o
WHINC=../..
WHOBJ=../../woofc-host.o
TINC=../..
TOBJ=../../woofc-thread.o
KINC=ks
KLIB=ks/kslib.a
SHEP_SRC=../../woofc-shepherd.c

CFLAGS=-g -I${SINC} -I${KINC} -I${MINC} -I${UINC}

H1=RHandler
H2=SHandler
H3=KHandler

all: c-runstest c-runstat cspot-runstat ${H1} ${H2} ${H3}

c-runstest: c-runstest.c c-runstest.h c-twist.o c-twist.h normal.o ${KLIB}
	${CC} ${CFLAGS} -DSTANDALONE -o c-runstest c-runstest.c c-twist.o ${KLIB} normal.o ${LIBS}

c-runstat: c-runstat.c c-runstest.o c-runstest.h c-twist.o c-twist.h
	${CC} ${CFLAGS} -o c-runstat c-runstat.c c-runstest.o c-twist.o ${KLIB} normal.o ${LIBS}

c-runstest.o: c-runstest.c c-runstest.h
	${CC} ${CFLAGS} -c c-runstest.c ${LIBS}

c-runstest-cspot.o: c-runstest.c c-runstest.h
	${CC} ${CFLAGS} -DHAS_WOOF -c c-runstest.c -o c-runstest-cspot.o ${LIBS}

c-twist.o: c-twist.c c-twist.h
	${CC} ${CFLAGS} -c c-twist.c

normal.o: normal.c normal.h
	${CC} ${CFLAGS} -c normal.c

${KLIB}:
	cd ks; make

cspot-runstat: cspot-runstat.c cspot-runstat.h c-twist.h c-twist.o ${LINCS} ${WINCS} ${SINCS} ${UINCS} ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	${CC} ${CFLAGS} -o cspot-runstat cspot-runstat.c c-twist.o ${WOBJ} ${WHOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp cspot-runstat ./cspot

${H1}: r-handler.c ${SHEP_SRC} ${LINCS} ${WINCS} ${SINCS} ${UINCS} c-twist.h ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	sed 's/WOOF_HANDLER_NAME/${H1}/g' ${SHEP_SRC} > ${H1}_shepherd.c
	${CC} ${CFLAGS} -c ${H1}_shepherd.c -o ${H1}_shepherd.o
	${CC} ${CFLAGS} -o ${H1} r-handler.c ${H1}_shepherd.o c-twist.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${H1} ./cspot

${H2}: s-handler.c c-runstest-cspot.o c-runstest.h ${SHEP_SRC} ${LINCS} ${WINCS} ${SINCS} ${UINCS} ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	sed 's/WOOF_HANDLER_NAME/${H2}/g' ${SHEP_SRC} > ${H2}_shepherd.c
	${CC} ${CFLAGS} -c ${H2}_shepherd.c -o ${H2}_shepherd.o
	${CC} ${CFLAGS} -o ${H2} s-handler.c ${H2}_shepherd.o c-runstest-cspot.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	mkdir -p cspot; cp ${H2} ./cspot

${H3}: k-handler.c ${SHEP_SRC} ${LINCS} ${WINCS} ${SINCS} ${UINCS} ${KINCS} normal.o normal.h ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS}
	sed 's/WOOF_HANDLER_NAME/${H3}/g' ${SHEP_SRC} > ${H3}_shepherd.c
	${CC} ${CFLAGS} -c ${H3}_shepherd.c -o ${H3}_shepherd.o
	${CC} ${CFLAGS} -o ${H3} k-handler.c ${H3}_shepherd.o normal.o ${WOBJ} ${SLIB} ${LOBJ} ${MLIB} ${ULIB} ${LIBS} ${KLIB}
	mkdir -p cspot; cp ${H3} ./cspot

clean:
	rm -f *.o c-runstest

