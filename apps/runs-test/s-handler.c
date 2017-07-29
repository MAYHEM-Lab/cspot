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

int SHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	FA *fa = (FA *)ptr;
	FA next_k;
	double stat;
	int err;
	FILE *fd;
	WOOF *r_wf;
	unsigned long start;
	unsigned long end;
	unsigned long ndx;
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

	start = WooFBack(r_wf,(fa->i+1)*(fa->sample_size-1));
	end = WooFBack(r_wf,fa->i*(fa->sample_size-1));
printf("SHandler: start: %lu, end: %lu\n",start,end);
fflush(stdout);

	v = (double *)malloc(sizeof(double)*fa->sample_size);
	i = 0;
	for(ndx=start; ndx <= end; ndx++) {
		WooFGet(r_wf,&v[i],ndx);
		i++;
	}

	stat = RunsStat(v,fa->sample_size);
	free(v);

	/*
	 * put the stat without a handler
	 */
	err = WooFPut(fa->stats,NULL,&stat);
	if(err < 0) {
		fprintf(stderr,"SHandler couldn't put stat\n");
		exit(1);
	}

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


	if(fa->i == (fa->count - 1)) {
		memcpy(&next_k,fa,sizeof(FA));
		err = WooFPut("Kargs","KHandler",&next_k);
		if(err < 0) {
			fprintf(stderr,"SHandler: couldn't create KHandler\n");
			exit(1);
		}
	}
	return(1);
}



