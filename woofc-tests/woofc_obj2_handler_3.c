#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj2.h"

int woofc_obj2_handler_3(WOOF *wf, unsigned long seq_no, void *ptr)
{

	OBJ2_EL el;
	char fbuf[4096];


	memcpy(&el,ptr,sizeof(el));


	if(el.counter < el.max) {
		el.counter++;
		fprintf(stdout,"handler3, calling put counter: %lu seq_no: %lu\n",el.counter,seq_no);
		fflush(stdout);
		if(WooFValidURI(el.next_woof)) {
			WooFPut(el.next_woof,"woofc_obj2_handler_2",&el);
		} else {
			WooFPut(wf->shared->filename,"woofc_obj2_handler_2",&el);
		}
	} else {
		fprintf(stdout,"handler3 done, counter: %lu seq_no: %lu\n",el.counter,seq_no);
		fflush(stdout);
	}


	return(1);

}
