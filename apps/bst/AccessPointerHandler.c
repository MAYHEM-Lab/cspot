#include <stdio.h>

#include "woofc.h"
#include "AccessPointer.h"

int AccessPointerHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

	AP *el = (AP *)ptr;
	fprintf(stdout,"ACCESS POINTER HANDLER -- WooF Name {%s} Seq No {%lu} DW_seq {%lu} LW_seq {%lu}\n",
			wf->shared->filename, seq_no, el->dw_seq_no, el->lw_seq_no);
	fflush(stdout);
	return(1);

}


