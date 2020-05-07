#include <stdio.h>

#include "woofc.h"
#include "dht.h"

typedef struct test_stc {
	char msg[256];
} TEST_EL;

int test_handler(WOOF *wf, unsigned long seq_no, void *ptr) {
	TEST_EL *el = (TEST_EL *)ptr;
	printf("from woof %s at %lu with string: %s\n", wf->shared->filename, seq_no, el->msg);
	return 1;
}