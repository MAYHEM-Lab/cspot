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

	stat = RunsStat(fa->r,fa->sample_size);

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



