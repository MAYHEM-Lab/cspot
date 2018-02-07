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

/*
 * dispatches incoming data
 */
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

#ifdef DEBUG
	fd = fopen("/cspot/req-handler.log","a+");
	fprintf(fd,"RegressPairReqHandler called on woof %s, type %c\n",
		rv->woof_name,rv->series_type);
	fflush(fd);
	fclose(fd);
#endif	

	MAKE_EXTENDED_NAME(finished_name,rv->woof_name,"finished");
	/*
	 * enable the first one
	 */
	if(wf_seq_no == 1) {
		WooFPut(finished_name,NULL,&(wf_seq_no));
	}
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
#if 0
	if(rv->series_type == 'p') {
		MAKE_EXTENDED_NAME(target_name,rv->woof_name,"predicted");
		MAKE_EXTENDED_NAME(progress_name,rv->woof_name,"progress");
		MAKE_EXTENDED_NAME(finished_name,rv->woof_name,"finished");

		seq_no = WooFGetLatestSeqno(finished_name);
		err = WooFGet(finished_name,(void *)&f_seq_no,seq_no);
		if(err < 0) {
			f_seq_no = 0;
			fprintf(stderr,"couldn't get f_seq_no %lu\n",seq_no);
			fflush(stdout);
		}
		seq_no = WooFGetLatestSeqno(progress_name);
		err = WooFGet(progress_name,(void *)&p_seq_no,seq_no);
		if(err < 0) {
			p_seq_no = 0;
			fprintf(stderr,"couldn't get p_seq_no %lu\n",seq_no);
			fflush(stdout);
		}
		if(p_seq_no <= f_seq_no) {
			p_rv = *rv;
			p_rv.seq_no = wf_seq_no;
			WooFPut(progress_name,NULL,(void *)&wf_seq_no);
			seq_no = WooFPut(target_name,
					 "RegressPairPredictedHandler",
					 (void *)&p_rv);
			if(WooFInvalid(seq_no)) {
				fprintf(stderr,
		"RegressPairReqHandler couldn't put predicted value\n");
			}
		}
	}
#endif

	return(1);
}
