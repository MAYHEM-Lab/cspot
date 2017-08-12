#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "ping-pong.h"

int pong(WOOF *wf, unsigned long seq_no, void *ptr)
{

	PP_EL el;


	memcpy(&el,ptr,sizeof(el));
	fprintf(stdout,"pong called, counter: %lu seq_no: %lu\n",el.counter,seq_no);
	fflush(stdout);

	if(el.counter < el.max) {
		el.counter++;
		if(WooFValidURI(el.next_woof)) {
			WooFPut(el.next_woof,"ping",&el);
		} else {
			WooFPut(wf->shared->filename,"ping",&el);
		}
	} else {
		fprintf(stdout,"pong done, counter: %lu seq_no: %lu\n",el.counter,seq_no);
		fflush(stdout);
	
	}

	return(1);

}
