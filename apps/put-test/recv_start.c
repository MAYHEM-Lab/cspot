#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "put-test.h"


int recv_start(WOOF *wf, unsigned long seq_no, void *ptr)
{

	PT_EL el;
	char target_name[4096];
	char log_name[4096];
	int err;

	memcpy(&el,ptr,sizeof(el));

	err = WooFCreate(el.target_name, el.element_size, el.history_size);
	if(err < 0) {
		fprintf(stderr,"recv_init: failed to create target woof %s\n",
			target_name);
		fflush(stderr);
		return(-1);
	}

	/*
	 * create a log WOOF to contain timestamps for received elements
	 */
	err = WooFCreate(el.log_name,sizeof(EX_LOG),el.history_size);
	if(err < 0) {
		fprintf(stderr,"recv_init: failed to create woof log\n");
		fflush(stderr);
		return(-1);
	}

	return(1);
}


