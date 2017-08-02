#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj2.h"


int woofc_obj2_handler_2(WOOF *wf, unsigned long seq_no, void *ptr)
{

	OBJ2_EL el;
	FILE *fd;
	char here[4096];
	char fbuf[4096];

	fd = fopen("/cspot-ns2/log-obj2","a");
	if(fd == NULL) {
		fprintf(stderr,"handler2 couldn't open log\n");
		fflush(stderr);
		return(0);
	}

	memcpy(&el,ptr,sizeof(el));

	fprintf(fd,"handler2 called, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
	fflush(fd);


	if(el.counter < 10) {
		el.counter++;
		fprintf(fd,"handler2, calling put counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(fd);

		if(WooFValidURI(el.next_woof)) {
			WooFPut(el.next_woof2,"woofc_obj2_handler_3",&el);
		} else {
			WooFPut(wf->shared->filename,"woofc_obj2_handler_3",&el);
		}
		el.counter++;
		fprintf(fd,"handler2, calling put counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(fd);
		if(WooFValidURI(el.next_woof)) {
			WooFPut(el.next_woof2,"woofc_obj2_handler_3",&el);
		} else {
			WooFPut(wf->shared->filename,"woofc_obj2_handler_3",&el);
		}
	} else {
		fprintf(fd,"handler2 done, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(fd);
	}

	fclose(fd);

	return(1);

}
