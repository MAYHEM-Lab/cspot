#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "cause.h"

int cause_recv(WOOF *wf, unsigned long seq_no, void *ptr)
{
	unsigned long latest_seq_no;

	CAUSE_EL *el = (CAUSE_EL *)ptr;
	fprintf(stdout, "cause_recv\n");
	fprintf(stdout, "from woof %s at %lu with string: %s\n",
			wf->shared->filename, seq_no, el->string);
	fflush(stdout);

	latest_seq_no = WooFGetLatestSeqno(wf->shared->filename);
	fprintf(stdout, "cause_recv: latest seq_no from %s: %lu\n", wf->shared->filename, latest_seq_no);
	fflush(stdout);
	
	return (1);
}
