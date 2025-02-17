CC=gcc
CFLAGS=-g

all: cmq-frame-selftest

cmq-frame-selftest: cmq-frame.c cmq-frame.h
	${CC} ${CFLAGS} -DSELFTEST -o cmq-frame-selftest cmq-frame.c 

clean:
	rm *.o cmq-frame-selftest

