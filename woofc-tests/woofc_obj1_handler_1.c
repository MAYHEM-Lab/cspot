#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "woofc-obj1.h"

int woofc_obj1_handler_1(WOOF *wf, unsigned long seq_no, void *ptr)
{

	OBJ1_EL *el = (OBJ1_EL *)ptr;
	fprintf(stdout,"hellow world\n");
	fprintf(stdout,"from %s at %lu with %s\n",
			wf->shared->filename, seq_no, el->string);
	fflush(stdout);
	return(1);

}
