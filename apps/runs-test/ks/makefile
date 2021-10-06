CC = gcc

RB = ./red_black
RBINC = ${RB}/red_black_2.h ${RB}/hval_2.h
RBLIB = red_black_2.o

CFLAGS = -g -I${RB}

all: ks cdf kslib.a

ks: ks.c simple_input.h simple_input.o cdf.h cdf.o ${RBINC} ${RBLIB}
	${CC} ${CFLAGS} -DSTANDALONE -o ks ks.c simple_input.o cdf.o ${RBLIB} -lm

simple_input.o: simple_input.c simple_input.h
	${CC} ${CFLAGS} -c simple_input.c

cdf.o: cdf.c cdf.h ${RBINC}
	${CC} ${CFLAGS} -c cdf.c

cdf: cdf.c cdf.h simple_input.h simple_input.o ${RBINC}
	${CC} ${CFLAGS} -DSTANDALONE -o cdf cdf.c simple_input.o ${RBLIB}

${RBLIB}: ${RB}/red_black_2.c ${RBINC}
	${CC} ${CFLAGS} -c ${RB}/red_black_2.c

kslib.a: ks.o simple_input.o cdf.o ${RB}/red_black_2.o
	ar -rc kslib.a ks.o ${RB}/red_black_2.o simple_input.o cdf.o red_black_2.o 
