CC=gcc
CFLAGS=-g -I../include
LIBS=-lyaml

all: woofc-keychain-test

woofc-keychain-test: woofc-keychain.c woofc-keychain.h
	${CC} ${CFLAGS} -DTEST -o woofc-keychain-test woofc-keychain.c ${LIBS}

clean:
	rm -f *.o woofc-keychain-test

