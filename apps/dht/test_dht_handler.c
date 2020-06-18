#include <stdio.h>
#include "dht_utils.h"

typedef struct test_stc {
	char msg[256];
	unsigned long sent;
} TEST_EL;

int test_dht_handler(char *topic_name, unsigned long seq_no, void *ptr) {
	TEST_EL *el = (TEST_EL *)ptr;
	printf("from %s[%lu], sent at %lu:\n", topic_name, seq_no, el->sent);
	printf("%s\n", el->msg);
	printf("took %lu ms\n", get_milliseconds() - el->sent);
	return 1;
}