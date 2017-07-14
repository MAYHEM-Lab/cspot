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
	memcpy(&el,ptr,sizeof(el));

	fd = fopen("/cspot/log-obj2","a");
	if(fd == NULL) {
		fprintf(stderr,"handler2 couldn't open log\n");
		fflush(stderr);
		return(0);
	}
		

	if(el.counter < 10) {
		fprintf(fd,"handler2, counter: %lu, seq_no: %lu\n",el.counter, seq_no);
		fflush(fd);
		el.counter++;
		WooFPut(wf->filename,"woofc_obj2_handler_3",&el);
		el.counter++;
		WooFPut(wf->filename,"woofc_obj2_handler_3",&el);
	}

	fclose(fd);

	return(1);

}
