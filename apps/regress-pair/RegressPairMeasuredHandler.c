#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "regress-pair.h"

#include "woofc.h"

FILE *fd;



int RegressPairMeasuredHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	REGRESSVAL result_rv;
	REGRESSCOEFF coeff_rv;
	char coeff_name[4096+64];
	char result_name[4096+64];
	unsigned long seq_no;
	double pred;
	int i;
	int err;
	struct timeval tm;

#ifdef DEBUG
        fd = fopen("/cspot/meas-handler.log","a+");
        fprintf(fd,"RegressPairMeasuredHandler called on woof %s, type %c\n",
                rv->woof_name,rv->series_type);
        fflush(fd);
        fclose(fd);
#endif  

	MAKE_EXTENDED_NAME(result_name,rv->woof_name,"result");
	MAKE_EXTENDED_NAME(coeff_name,rv->woof_name,"coeff");


	memcpy(result_rv.woof_name,rv->woof_name,sizeof(result_rv.woof_name));

	seq_no = WooFGetLatestSeqno(coeff_name); 

	err = WooFGet(coeff_name,(void *)&coeff_rv,seq_no);
	if(err < 0) {
		fprintf(stderr,
			"RegressPairPredictedHandler couldn't get count back from %s\n",coeff_name);
		return(-1);
	}

	pred = (coeff_rv.slope * rv->value.d) + coeff_rv.intercept;
	result_rv.value.d = pred;
	result_rv.tv_sec = rv->tv_sec;
	result_rv.tv_usec = rv->tv_usec;
	result_rv.series_type = 'r';
/*
printf("PRED: %lu %f measured: %f\n",ntohl(rv->tv_sec),ntohl(rv->tv_usec),pred,rv->value.d);
fflush(stdout);
*/

	seq_no = WooFPut(result_name,NULL,(void *)&result_rv);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,
			"RegressPairPredictedHandler: couldn't put result to %s\n",result_name);
		return(-1);
	}

	return(1);
}

