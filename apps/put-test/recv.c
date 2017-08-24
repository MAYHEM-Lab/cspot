#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "put-test.h"


/*
 * put on the target and not on the WOOF with the args
 */
int recv(WOOF *wf, unsigned long seq_no, void *ptr)
{
	WOOF *woof_log;
	char log_name[4096];
	unsigned long seq_no;
	EX_LOG elog;

	gettimeofday(&(elog.tm),NULL);
	pl = (PL *)ptr;
	elog.exp_seq_no = pl->exp_seq_no;
	

	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s.%s",wf->shared->filename,"log");

	seq_no = WooFPut(log_name,NULL,&elog);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"recv: couldn't log %lu\n",wf->shared->seq_no);
		fflush(stderr);
		return(-1);
	}

	return(1);
}


