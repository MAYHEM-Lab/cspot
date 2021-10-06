#include <stdio.h>
#include "woofc.h"

typedef struct test_stc {
	char msg[256];
} TEST_EL;

int test_raft_handler(WOOF *wf, unsigned long seq_no, void *ptr) {
	TEST_EL *el = (TEST_EL *)ptr;
	printf("%s\n", el->msg);
	return 1;
}