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


int RHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	FA *fa = (FA *)ptr;
	FA next_r;
	FA next_s;
	int err;
	double r;
	struct timeval tm;
	unsigned long seq_no;
	unsigned long r_seq_no;

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
	seq_no = WooFPut("Rargs","RHandler",&next_r);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"RHandler couldn't put RHandler\n");
		exit(1);
	}

	/*
	 * put put the random value with no handler
	 */
	r_seq_no = WooFPut(fa->r,NULL,&r);
	if(WooFInvalid(r_seq_no)) {
		fprintf(stderr,"RHandler couldn't put random value\n");
		exit(1);
	}

printf("RHandler: i: %d, seq_no: %lu, r: %f\n",fa->i, r_seq_no, r);
fflush(stdout);


	/*
	 * if the buffer is full, create an SThread
	 */
	if(fa->j == (fa->sample_size - 1)) {
		/*
	 	* launch the stat handler
	 	*/
		memcpy(&next_s,fa,sizeof(FA));
		next_s.seq_no = r_seq_no;
		seq_no = WooFPut("Sargs","SHandler",&next_s);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,"RHandler couldn't put SHandler\n");
			exit(1);
		}

	}

	return(1);
}
