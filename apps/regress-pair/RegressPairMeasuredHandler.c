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

#include "regress-pair.h"

#include "woofc.h"
#include "mioarray.h"
#include "ssa-decomp.h"

FILE *fd;

int RegressPairMeasuredHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	REGRESSVAL m_rv;
	REGRESSVAL result_rv;
	REGRESSCOEFF coeff_rv;
	char coeff_name[4096+64];
	char result_name[4096+64];
	char measured_name[4096+64];
	unsigned long seq_no;
	double pred;
	int i;
	int err;
	struct timeval tm;
	unsigned int size;
	Array2D *unsmoothed;
	Array2D *smoothed;

#ifdef TIMING
	struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);
#endif

#ifdef DEBUG
        fd = fopen("/cspot/meas-handler.log","a+");
        fprintf(fd,"RegressPairMeasuredHandler called on woof %s, type %c\n",
                rv->woof_name,rv->series_type);
        fflush(fd);
        fclose(fd);
#endif  

	MAKE_EXTENDED_NAME(measured_name,rv->woof_name,"measured");
	MAKE_EXTENDED_NAME(result_name,rv->woof_name,"result");
	MAKE_EXTENDED_NAME(coeff_name,rv->woof_name,"coeff");
	sprintf(measured_name, "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test.measured");
	sprintf(result_name, "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test.result");
	sprintf(coeff_name, "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/test.coeff");

	memcpy(result_rv.woof_name,rv->woof_name,sizeof(result_rv.woof_name));

	seq_no = WooFGetLatestSeqno(coeff_name); 
	err = WooFGet(coeff_name,(void *)&coeff_rv,seq_no);
	if(err < 0) {
		fprintf(stderr,
			"RegressPairMeasuredHandler couldn't get count back from %s\n",coeff_name);
		fflush(stderr);
		return(-1);
	}

	size = (wf_seq_no - coeff_rv.earliest_seq_no + 1);
	if(coeff_rv.lags > 0) {
		unsmoothed = MakeArray2D(size,2);
		if(unsmoothed == NULL) {
			fprintf(stderr,"RegressPairMeasuredHandler: no space for unsmoothed size %d\n",size);
			fflush(stderr);
			return(-1);
		}
		seq_no = coeff_rv.earliest_seq_no;
		i = 0;
		while(seq_no < wf_seq_no) {
			err = WooFGet(measured_name,&m_rv,seq_no);
			if(err < 0) {
				fprintf(stderr,
"RegressPairMeasuredHandler: couldn't read seqno %lu from %s\n",
					seq_no,
					measured_name);
				fflush(stderr);
				unsmoothed->ydim = i;
				break;
			}
			MAKETS(unsmoothed->data[i*2+0],&m_rv);
			unsmoothed->data[i*2+1] = m_rv.value.d;
			i++;
			seq_no++;
		}
		MAKETS(unsmoothed->data[i*2+0],rv);
		unsmoothed->data[i*2+1] = rv->value.d;

		smoothed = SSASmoothSeries(unsmoothed,coeff_rv.lags);
		FreeArray2D(unsmoothed);
		if(smoothed == NULL) {
			fprintf(stderr,
"RegressPairMeasuredHandler: %s couldn't get ssa series for lags %d\n",
				measured_name,
				coeff_rv.lags);
			fflush(stderr);
			pred = (coeff_rv.slope * rv->value.d) + coeff_rv.intercept;
		} else {
			i = smoothed->ydim-1;
			pred = (coeff_rv.slope * smoothed->data[i*2+1]) + 
				coeff_rv.intercept;
			FreeArray2D(smoothed);
		}
	} else {
		pred = (coeff_rv.slope * rv->value.d) + coeff_rv.intercept;
	}
	result_rv.value.d = pred;
	result_rv.tv_sec = rv->tv_sec;
	result_rv.tv_usec = rv->tv_usec;
	result_rv.series_type = 'r';

printf("PRED (%s): lags: %d, %lu %f measured: %f\n", measured_name,
coeff_rv.lags,ntohl(rv->tv_sec),ntohl(rv->tv_usec),pred,rv->value.d);
fflush(stdout);

	seq_no = WooFPut(result_name,NULL,(void *)&result_rv);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,
			"RegressPairMeasuredHandler: couldn't put result to %s\n",result_name);
		return(-1);
	}

#ifdef TIMING
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("TIMING:RegressPairMeasuredHandler: %f\n", elapsedTime);
	fflush(stdout);
#endif
	return(1);
}

