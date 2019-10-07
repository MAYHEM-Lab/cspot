#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "dht.h"

int test(WOOF *wf, unsigned long seq_no, void *ptr)
{
	TEST_EL *el = (TEST_EL *)ptr;
	printf("from woof %s at %lu with string: %s\n", wf->shared->filename, seq_no, el->msg);
	fflush(stdout);
	return 1;
}
