#define TIMING

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#include "regress-pair.h"

#include "woofc.h"

FILE *fd;

int RegressPairReqHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	REGRESSVAL p_rv;
	char target_name[4096+64];
	char progress_name[4096+64];
	char finished_name[4096+64];
	unsigned long seq_no;
	unsigned long f_seq_no;
	unsigned long p_seq_no;
	int err;

#ifdef TIMING
	struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);
#endif

#ifdef DEBUG
	fd = fopen("/cspot/req-handler.log","a+");
	fprintf(fd,"RegressPairReqHandler called on woof %s, type %c\n",
		rv->woof_name,rv->series_type);
	fflush(fd);
	fclose(fd);
#endif	

	if(rv->series_type == 'm') {
		MAKE_EXTENDED_NAME(target_name,rv->woof_name,"measured");
		seq_no = WooFPut(target_name,
				 "RegressPairMeasuredHandler", 
				 ptr);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,
	"RegressPairReqHandler couldn't put measured value\n");
		}
	}

	if(rv->series_type == 'p') {
		MAKE_EXTENDED_NAME(target_name,rv->woof_name,"predicted");
		seq_no = WooFPut(target_name,
				 "RegressPairPredictedHandler",
				 ptr);
/*
printf("REQ: just put %lu to predicted\n",seq_no);
fflush(stdout);
*/
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,
	"RegressPairReqHandler couldn't put predicted value\n");
		}
	}

#ifdef TIMING
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("TIMING:RegressPairReqHandler: %f\n", elapsedTime);
	fflush(stdout);
#endif
	return(1);
}

