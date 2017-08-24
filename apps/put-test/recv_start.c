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
	WOOF *target;
	WOOF *woof_log;
	char target_name[4096];
	char log_name[4096];

	memcpy(&el,ptr,sizeof(el));

	target = WooFCreate(el.target_name, el.element_size, el.history_size);
	if(target == NULL) {
		fprintf(stderr,"recv_init: failed to create target woof %s\n",
			target_name);
		fflush(stderr);
		return(-1);
	}

	WooFFree(target);

	/*
	 * create a log WOOF to contain timestamps for received elements
	 */
	woof_log = WooFCreate(el.log_name,sizeof(EX_LOG),el.history_size);
	if(woof_log == NULL) {
		fprintf(stderr,"recv_init: failed to create woof log\n");
		fflush(stderr);
		WooFFree(target);
		return(-1);
	}

	WooFFree(woof_log);

	return(1);
}


