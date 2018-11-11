#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "repair.h"

int repair(WOOF *wf, unsigned long seq_no, void *ptr)
{

	REPAIR_EL *el = (REPAIR_EL *)ptr;
	fprintf(stdout,"repair\n");
	fprintf(stdout,"from woof %s at %lu with string: %s\n",
			wf->shared->filename, seq_no, el->string);
	fflush(stdout);
	return(1);

}
