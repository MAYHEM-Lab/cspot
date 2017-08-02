#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "woofc-access.h"
#include "woofc-obj2.h"

int woofc_obj2_handler_1(WOOF *wf, unsigned long seq_no, void *ptr)
{

	OBJ2_EL *el = (OBJ2_EL *)ptr;
	char fbuf[4096];

	if(el->counter < 10) {
		printf("name: %s, counter: %lu seq_no: %lu\n",wf->shared->filename, el->counter, seq_no);
		fflush(stdout);
		el->counter++;
		printf("name: %s, calling WooFPut\n",wf->shared->filename);
		fflush(stdout);
		if(WooFValidURI(el->next_woof)) {
			WooFPut(el->next_woof,"woofc_obj2_handler_1",(void *)el);
		} else {
			WooFPut(wf->shared->filename,"woofc_obj2_handler_1",(void *)el);
		}
	}

	return(1);

}
