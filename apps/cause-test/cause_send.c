#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "cause.h"

int cause_send(WOOF *wf, unsigned long seq_no, void *ptr)
{
	int err;
	unsigned long ndx;
	CAUSE_EL *el = (CAUSE_EL *)ptr;
	fprintf(stdout, "cause_send\n");
	fprintf(stdout, "from woof %s at %lu with string: %s\n",
			wf->shared->filename, seq_no, el->string);
	fflush(stdout);

	ndx = WooFPut(el->dst, "cause_recv", el);

	if (WooFInvalid(ndx))
	{
		fprintf(stderr, "WooFPut failed for %s\n", el->dst);
		fflush(stderr);
		exit(1);
	}

	err = WooFGet(el->dst, el, ndx);
	if (err < 0)
	{
		fprintf(stderr, "WooFGet failed for %s\n", el->dst);
		fflush(stderr);
		exit(1);
	}

	return (1);
}
