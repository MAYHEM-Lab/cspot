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
	char f_name[4096];
	unsigned long p_seq_no;
	EX_LOG elog;
	PL *pl;
	char *cptr;
	char *p;
	int i;

	gettimeofday(&(elog.tm),NULL);
	pl = (PL *)ptr;
	elog.exp_seq_no = pl->exp_seq_no;
	

	memset(log_name,0,sizeof(log_name));
	memset(f_name,0,sizeof(f_name));
	cptr = strstr(wf->shared->filename,".target");
	if(cptr == NULL) {
		fprintf(stderr,"recv: cpouldn't find base iname in %s\n",
			wf->shared->filename);
		fflush(stderr);
		return(-1);
	}

	i = 0;
	p = wf->shared->filename;
	while(&(p[i]) != cptr) {
		if(i >= sizeof(f_name)) {
			fprintf(stderr,"recv: cpouldn't bad name in %s\n",
				wf->shared->filename);
			fflush(stderr);
			return(-1);
		}
		f_name[i] = p[i];
		i++;
	}

	
	sprintf(log_name,"%s.%s",f_name,"log");

	p_seq_no = WooFPut(log_name,NULL,&elog);
	if(WooFInvalid(p_seq_no)) {
		fprintf(stderr,"recv: couldn't log %lu\n",wf->shared->seq_no);
		fflush(stderr);
		return(-1);
	}

	return(1);
}


