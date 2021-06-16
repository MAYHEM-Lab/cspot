CC = gcc
CFLAGS = -g
LIBS = -lm

LIB_INSTALL=../lib
INC_INSTALL=../include

all: redblack.test rb_string_test textlist.test simple_input.test convert_time ptime aggr libutils.a

convert_time: convert_time.c
	${CC} ${CFLAGS} -DSTANDALONE -o convert_time convert_time.c

ptime: ptime.c
	${CC} ${CFLAGS} -o ptime ptime.c

aggr: aggr.c simple_input.h simple_input.o
	${CC} ${CFLAGS} -o aggr aggr.c simple_input.o

redblack.test: redblack.c redblack.h dlist.h dlist.o hval.h
	${CC} ${CFLAGS} -DTEST -DDEBUG_RB -o redblack.test redblack.c dlist.o

textlist.test: redblack.o redblack.h dlist.o dlist.h hval.h textlist.c textlist.h
	${CC} ${CFLAGS} -DTEST -o textlist.test textlist.c dlist.o redblack.o

rb_string_test: redblackdebug.o redblackdebug.h redblack.h rb_string_test.c dlist.o dlist.h hval.h redblack.c
	${CC} ${CFLAGS} -DDEBUG_RB -o rb_string_test rb_string_test.c redblackdebug.o dlist.o 

simple_input.test: simple_input.c simple_input.h
	${CC} ${CFLAGS} -DTEST -o simple_input.test simple_input.c

redblackdebug.o: redblack.c hval.h redblack.h redblackdebug.h dlist.h
	${CC} ${CFLAGS} -DDEBUG_RB -c redblack.c -o redblackdebug.o

convert_time.o: convert_time.c convert_time.h
	${CC} ${CFLAGS} -c convert_time.c

dlist.o: dlist.h dlist.c
	${CC} ${CFLAGS} -c dlist.c

redblack.o: redblack.c redblack.h hval.h
	${CC} ${CFLAGS} -c redblack.c

textlist.o: textlist.c textlist.h hval.h redblack.h redblack.o dlist.h dlist.o
	${CC} ${CFLAGS} -c textlist.c

simple_input.o: simple_input.c simple_input.h
	${CC} ${CFLAGS} -c simple_input.c

libutils.a: textlist.o dlist.o redblack.o simple_input.o convert_time.o
	ar -cr libutils.a textlist.o dlist.o redblack.o simple_input.o convert_time.o

install:
	install -C -d -S libutils.a ${LIB_INSTALL}
	install -C -d -S hval.h ${INC_INSTALL}
	install -C -d -S dlist.h ${INC_INSTALL}
	install -C -d -S redblack.h ${INC_INSTALL}
	install -C -d -S textlist.h ${INC_INSTALL}

clean:
	rm -f *.o *.test *.a

