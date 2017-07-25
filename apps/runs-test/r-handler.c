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

uint32_t Seed;


int RHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	FA *fa = (FA *)ptr;
	FA next_r;
	FA next_s;
	int err;
	double r;
	struct timeval tm;

	/*
	 * sanity checks
	 */
	if(fa->i >= fa->count) {
		return(1);
	}

	if(fa->j >= fa->sample_size) {
		return(1);
	}

	gettimeofday(&tm,NULL);
	Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	CTwistInitialize(Seed);
	/*
	 * fix this to use woof for random state
	 */
	r = CTwistRandom();

	memcpy(&next_r,fa,sizeof(FA));
	if(next_r.j == (fa->sample_size-1)) {
		next_r.i++;
		next_r.j = 0;
	} else {
		next_r.j++;
	}

	/*
	 * generate next random number
	 */
	err = WooFPut("Rargs","RHandler",&next_r);
	if(err < 0) {
		fprintf(stderr,"RHandler couldn't put RHandler\n");
		exit(1);
	}

printf("RHandler: i: %d, j: %d, putting %f to %s\n",fa->i,fa->j,r,fa->r);
fflush(stdout);
	/*
	 * put put the random value with no handler
	 */
	err = WooFPut(fa->r,NULL,&r);
	if(err < 0) {
		fprintf(stderr,"RHandler couldn't put random value\n");
		exit(1);
	}


	/*
	 * if the buffer is full, create an SThread
	 */
	if(fa->j == (fa->sample_size - 1)) {
printf("RHandler putting SHandler\n");
fflush(stdout);
		/*
	 	* launch the stat handler
	 	*/
		memcpy(&next_s,fa,sizeof(FA));
		err = WooFPut("Sargs","SHandler",&next_s);
		if(err < 0) {
			fprintf(stderr,"RHandler couldn't put SHandler\n");
			exit(1);
		}

	}

	return(1);
}
