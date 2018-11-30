#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "cause.h"

int cause_recv(WOOF *wf, unsigned long seq_no, void *ptr)
{
	CAUSE_EL *el = (CAUSE_EL *)ptr;
	fprintf(stdout, "cause_recv\n");
	fprintf(stdout, "from woof %s at %lu with string: %s\n",
			wf->shared->filename, seq_no, el->string);
	fflush(stdout);
	return (1);
}
