#include <stdio.h>

typedef struct test_stc {
	char msg[256];
} TEST_EL;

int test_handler(char *woof_name, unsigned long seq_no, void *ptr) {
	TEST_EL *el = (TEST_EL *)ptr;
	printf("from %s at %lu with string: %s\n", woof_name, seq_no, el->msg);
	return 1;
}