CC=gcc
LIBS=sema.o -lpthread
CFLAGS=-g

all: dispatch-fork test-handler

dispatch-fork: dispatch-fork.c sema.o sema.h
	${CC} ${CFLAGS} -o dispatch-fork dispatch-fork.c ${LIBS}

test-handler: test-handler.c
	${CC} ${CFLAGS} -o test-handler test-handler.c ${LIBS}

sema.o: sema.c sema.h
	${CC} ${CFLAGS} -c sema.c

