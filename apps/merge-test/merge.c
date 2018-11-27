#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "merge.h"

int merge(WOOF *wf, unsigned long seq_no, void *ptr)
{

	MERGE_EL *el = (MERGE_EL *)ptr;
	fprintf(stdout,"hello world\n");
	fprintf(stdout,"from woof %s at %lu with string: %s\n",
			wf->shared->filename, seq_no, el->string);
	fflush(stdout);
	return(1);

}
