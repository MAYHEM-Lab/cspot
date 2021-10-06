#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "cause.h"
#include "woofc.h"

int cause_recv(WOOF *wf, unsigned long seq_no, void *ptr)
{
	unsigned long latest_seq_no;

	CAUSE_EL *el = (CAUSE_EL *)ptr;
	fprintf(stdout, "cause_recv\n");
	fprintf(stdout, "from woof %s at %lu with string: %s\n",
			WoofGetFileName(wf), seq_no, el->string);
	fflush(stdout);

	latest_seq_no = WooFGetLatestSeqno(WoofGetFileName(wf));
	fprintf(stdout, "cause_recv: latest seq_no from %s: %lu\n", WoofGetFileName(wf), latest_seq_no);
	fflush(stdout);
	
	return (1);
}
