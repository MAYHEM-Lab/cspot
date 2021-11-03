
#include <stdio.h>

#include "woofc.h"
#include "hw.h"

int hw_stress(WOOF *wf, unsigned long seq_no, void *ptr)
{

	HW_EL *el = (HW_EL *)ptr;
	// fprintf(stdout,"hello world\n");
	// fprintf(stdout,"from woof %s at %lu with string: %s\n",
	// 		wf->shared->filename, seq_no, el->string);
	// fflush(stdout);
	return(1);

}
