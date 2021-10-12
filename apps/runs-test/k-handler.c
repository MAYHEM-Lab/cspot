#define TIMING

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "c-twist.h"
#include "c-runstest.h"
#include "normal.h"
#include "ks.h"

#include "woofc.h"
#include "cspot-runstat.h"


int KHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	FA *fa = (FA *)ptr;
	FILE *fd;
	double kstat;
	int i;
	void *normal;
	void *local;
	int err;
	double n;
	double incr;
	double value;
	double critical;
	double s;
	unsigned long seq_no;
	int count;
	WOOF *s_wf;

#ifdef TIMING
	struct timeval t1, t2;
	double elapsedTime;
	gettimeofday(&t1, NULL);
#endif

	/*
	 * sanity check
	 */
	if(fa->i > fa->count) {
		return(1);
	}


	/*
	 * generate a Normal(0,1) to compare with
	 */
	err = InitDataSet(&normal,1);
	if(err < 0) {
		fprintf(stderr,"KHandler couldn't init normal data set\n");
		exit(1);
	}

	err = InitDataSet(&local,1);	/* for KS routine */
	if(err < 0) {
		fprintf(stderr,"KHandler couldn't init local data set\n");
		exit(1);
	}

	s_wf = WooFOpen(fa->stats);
	if(wf == NULL) {
		fprintf(stderr,"KHandler couldn't open stats woof %s\n",
			fa->stats);
		exit(1);
	}

	incr = 1.0 / (double)fa->count;
	value = 0.0000000001;
	for(i=0; i < fa->count; i++) {
		n = InvNormal(value,0.0,1.0);
		WriteData(normal,1,&n);
		value += incr;
		if(value >= 1.0) {
			value = 0.9999999999;
		}
	}

	count = 0;
printf("Khandler: seq_no: %lu, count: %d\n",fa->seq_no,fa->count);
fflush(stdout);
	for(seq_no=fa->seq_no; count < fa->count; seq_no--) {
		err = WooFGet(WoofGetFileName(s_wf),&s,seq_no);
		if(err < 0) {
			fprintf(stderr,"WooFRead failed at %lu for %s\n",
					seq_no,fa->stats);
			exit(1);
		}
		WriteData(local,1,&s);
		count++;
	}


	critical = KSCritical(fa->alpha,(double)fa->count,(double)fa->count);
	kstat = KS(local,1,normal,1);

	FreeDataSet(local);


	if(fa->logfile[0] != 0) {
		fd = fopen(fa->logfile,"a");
		if(fd == NULL) {
			fprintf(stderr,
			"KHandler: ERROR couldn't open log file %s\n",
			fa->logfile);
			fflush(stderr);
			fd = stdout;
		}
	} else {
		fd = stdout;
	}
	fprintf(fd,"KS stat: %f alpha: %f critical value: %f\n",
		kstat, fa->alpha, critical);
	if(fa->logfile[0] != 0) {
		fclose(fd);
	} else {
		fflush(stdout);
	}

	WooFDrop(s_wf);

#ifdef TIMING
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("Timing:KHandler: %f ms\n", elapsedTime);
	fflush(stdout);
#endif
	return(1);
}
