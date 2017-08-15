#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj2.h"


int woofc_obj2_handler_2(WOOF *wf, unsigned long seq_no, void *ptr)
{

	OBJ2_EL el;
	char here[4096];
	char fbuf[4096];
	OBJ2_EL r_el;
	unsigned long r_seq_no;
	int err;


	memcpy(&el,ptr,sizeof(el));

	fprintf(stdout,"handler2 called, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
	fflush(stdout);


	if(el.counter < el.max) {
		el.counter++;
		fprintf(stdout,"handler2, calling put counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(stdout);

		if(WooFValidURI(el.next_woof)) {
//			seq_no = WooFPut(el.next_woof2,"woofc_obj2_handler_3",&el);
			seq_no = WooFPut(el.next_woof2,NULL,&el);
		} else {
//			WooFPut(wf->shared->filename,"woofc_obj2_handler_3",&el);
			WooFPut(wf->shared->filename,NULL,&el);
		}
		el.counter++;
		fprintf(stdout,"handler2, calling put counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(stdout);
		if(WooFValidURI(el.next_woof)) {
			r_seq_no = WooFPut(el.next_woof2,"woofc_obj2_handler_3",&el);
			/*
			 * read it back to make sure
			 */
#if 0
			err = WooFGet(el.next_woof2,&r_el,r_seq_no);
			if(err < 0) {
				fprintf(stderr,"handler2: WooFGet of %lu failed\n",r_seq_no);
				fflush(stderr);
			} else if(memcmp(&el,&r_el,sizeof(el)) != 0) {
				fprintf(stderr,"handler2: WooFGet of %lu failed to compare\n",r_seq_no);
				fflush(stderr);
			} else {
				fprintf(stdout,
				   "handler2, verified put at %lu\n", r_seq_no);
				fflush(stdout);
			}
#endif
		} else {
			WooFPut(wf->shared->filename,"woofc_obj2_handler_3",&el);
		}
	} else {
		fprintf(stdout,"handler2 done, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(stdout);
	}

	return(1);

}
