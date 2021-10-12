CC=gcc
LIBS=sema.o -lpthread
CFLAGS=-g

all: dispatch-fork test-handler dispatch-pipe

dispatch-fork: dispatch-fork.c sema.o sema.h
	${CC} ${CFLAGS} -o dispatch-fork dispatch-fork.c ${LIBS}

dispatch-pipe: dispatch-pipe.c sema.o sema.h fork-proc
	${CC} ${CFLAGS} -o dispatch-pipe dispatch-pipe.c ${LIBS}

fork-proc: fork-proc.c
	${CC} ${CFLAGS} -o fork-proc fork-proc.c ${LIBS}

test-handler: test-handler.c
	${CC} ${CFLAGS} -o test-handler test-handler.c ${LIBS}

sema.o: sema.c sema.h
	${CC} ${CFLAGS} -c sema.c

