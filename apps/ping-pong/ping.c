#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "ping-pong.h"


int ping(WOOF *wf, unsigned long seq_no, void *ptr)
{

	PP_EL el;

	memcpy(&el,ptr,sizeof(el));

	fprintf(stdout,"ping called, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
	fflush(stdout);


	if(el.counter < 10) {
		el.counter++;

		if(WooFValidURI(el.next_woof2)) {
			WooFPut(el.next_woof2,"pong",&el);
		} else {
			WooFPut(wf->shared->filename,"pong",&el);
		}
	} else {
		fprintf(stdout,"ping done, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(stdout);
	}

	return(1);

}
