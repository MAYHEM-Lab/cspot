#define HAS_WOOF
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#include "c-twist.h"
#include "c-runstest.h"
#include "normal.h"
#include "ks.h"

#include "woofc.h"
#include "cspot-runstat.h"

int SHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	FA *fa = (FA *)ptr;
	FA next_k;
	double stat;
	int err;
	FILE *fd;
	WOOF *r_wf;
	unsigned long start;
	unsigned long end;
	unsigned long seq_no;
	int i;
	double *v;


	/*
	 * sanity check
	 */
	if(fa->i >= fa->count) {
		return(1);
	}

	/*
	 * sanity check
	 */
	if(fa->j != (fa->sample_size - 1)) {
		return(1);
	}

	r_wf = WooFOpen(fa->r);
	if(r_wf == NULL) {
		fprintf(stderr,"SHandler: couldn't open %s for rvals\n",fa->r);
		return(-1);
	}

	end = fa->seq_no;
	start = end - (fa->sample_size-1);
printf("SHandler: start: %lu, end: %lu\n",start,end);
fflush(stdout);

	v = (double *)malloc(sizeof(double)*fa->sample_size);
	i = 0;
	for(seq_no=start; seq_no <= end; seq_no++) {
		WooFRead(r_wf,&v[i],seq_no);
		i++;
	}

	stat = RunsStat(v,fa->sample_size);
	free(v);

	/*
	 * put the stat without a handler
	 */
	seq_no = WooFPut(fa->stats,NULL,&stat);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"SHandler couldn't put stat\n");
		exit(1);
	}
printf("shandler: seq_no %lu, i: %d\n",seq_no,fa->i);
fflush(stdout);

	if(fa->logfile[0] != 0) {
		fd = fopen(fa->logfile,"a");
	} else {
		fd = stdout;
	}
	fprintf(fd,"i: %d stat: %f\n",fa->i,stat);
	if(fa->logfile[0] != 0) {
		fclose(fd);
	} else {
		fflush(stdout);
	}


	if(seq_no == fa->count) {
		memcpy(&next_k,fa,sizeof(FA));
		next_k.seq_no = seq_no;
printf("SHandler: putting seq_no: %lu\n",seq_no);
fflush(stdout);
		seq_no = WooFPut(fa->kargs,"KHandler",&next_k);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,"SHandler: couldn't create KHandler\n");
			exit(1);
		}
	}
	return(1);
}



