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

/*
 * dispatches incoming data
 */
int RegressPairReqHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	char target_name[4096+64];
	unsigned long seq_no;

	if(rv->series_type == MEASURED) {
		MAKE_EXTENDED_NAME(target_name,rv->woof_name,"measured");
		seq_no = WooFPut(target_name,
				 NULL, /* no handler for measurement */
				 ptr);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,
	"RegressPairReqHandler couldn't put measured value\n");
		}
	}

	if(rv->series_type == PREDICTED) {
		MAKE_EXTENDED_NAME(target_name,rv->woof_name,"predicted");
		seq_no = WooFPut(target_name,
				 "RegressPairPredictedHandler",
				 ptr);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,
	"RegressPairReqHandler couldn't put predicted value\n");
		}
	}

	return(1);
}
