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

double *ComputeMatchces(char *predicted_name,char *measured_name,int count_back)
{
	double *matches;
	int i;
	int err;
	unsigned long m_seq_no;
	unsigned long p_seq_no;
	unsigned long seq_no;
	int count;
	double p_ts;
	double m_ts;
	double next_ts;
	REGRESSVAL p_rv;
	REGRESSVAL m_rv;
	REGRESSVAL next_rv;

	matches = (double *)malloc(count_back * sizeof(double) * 2);
	if(matches == NULL) {
		return(NULL);
	}

	memset(matches,0,count_back * sizeof(double) * 2);

	m_seq_no = WooFGetLatestSeqno(measured_name);
	if(WooFInvalid(m_seq_no)) {
		fprintf(stderr,"no latest seq no in %s\n",
			measured_name);
		fflush(stderr);
		free(matches);
		return(NULL);
	}
	p_seq_no = WooFGetLatestSeqno(predicted_name);
	if(WooFInvalid(m_seq_no)) {
		fprintf(stderr,"no latest seq no in %s\n",
			predicted_name);
		fflush(stderr);
		free(matches);
		return(NULL);
	}

	if((int)(p_seq_no - count_back) < 0) {
		fprintf(stderr,"not enough history for %d countback in %s\n",
			count_back,predicted_name);
		fflush(stderr);
		free(matches);
		return(NULL);
	}
	if((int)(m_seq_no - count_back) < 0) {
		fprintf(stderr,"not enough history for %d countback in %s\n",
			count_back,measured_name);
		fflush(stderr);
		free(matches);
		return(NULL);
	}

	/*
	 * get the earliest value from the predicted series
	 */
	count = count_back;
	err = WooFGet(predicted_name,(void *)&p_rv,(p_seq_no - count));
	if(err < 0) {
		fprintf(stderr,"couldn't get earliest at %lu in %s\n",
			p_seq_no-count,predicted_name);
		fflush(stderr);
		free(matches);
		return(NULL);
	}

	/*
	 * walk back in the measured series and look for entry that is
	 * immediately before predicted value in terms of time stamp
	 */
	seq_no = m_seq_no;
	while((int)seq_no >= 0) {
		err = WooFGet(measured_name,(void *)&m_rv,seq_no);
		if(err < 0) {
			fprintf(stderr,"couldn't get measured at %lu in %s\n",
				seq_no,measured_name);
			fflush(stderr);
			free(matches);
			return(NULL);
		}

		p_ts = (double)ntohl(p_rv.tv_sec)+(double)(ntohl(p_rv.tv_usec) / 1000000.0);
		m_ts = (double)ntohl(m_rv.tv_sec)+(double)(ntohl(m_rv.tv_usec) / 1000000.0);
		if(m_ts < p_ts) {
			break;
		}
		seq_no--;
	}

	if((int)seq_no < 0) {
		fprintf(stderr,"couldn't find ts earliesr than %10.0f in %s\n",
			p_ts,measured_name);
		fflush(stderr);
		free(matches);
		return(NULL);
	}

	/*
	 * now bracket the earliest entry
	 */
	seq_no++;
	err = WooFGet(measured_name,(void *)&next_rv,seq_no);
	if(err < 0) {
		fprintf(stderr,"couldn't get next rv from %s at %lu\n",
				measured_name,seq_no);
		fflush(stderr);
		free(matches);
		return(NULL);
	}

	i = 0;
	next_ts = (double)ntohl(next_rv.tv_sec)+(double)(ntohl(next_rv.tv_usec) / 1000000.0);
	while(seq_no <= m_seq_no) {
		if(fabs(p_ts-next_ts) < fabs(p_ts-m_ts)) {
			matches[i*2+0] = p_rv.value.d;
			matches[i*2+1] = next_rv.value.d;
		} else {
			matches[i*2+0] = p_rv.value.d;
			matches[i*2+1] = m_rv.value.d;
		}
		
		i++;
		count--;
		err = WooFGet(predicted_name,(void *)&p_rv,(p_seq_no - count));
		if(err < 0) {
			fprintf(stderr,"couldn't get current at %lu in %s\n",
			p_seq_no-count,predicted_name);
			fflush(stderr);
			free(matches);
			return(NULL);
		}
		p_ts = (double)ntohl(p_rv.tv_sec)+(double)(ntohl(p_rv.tv_usec) / 1000000.0);
		
		memcpy(&m_rv,&next_rv,sizeof(m_rv));
		seq_no++;
		err = WooFGet(measured_name,(void *)&next_rv,seq_no);
		if(err < 0) {
			fprintf(stderr,"couldn't get next in rv from %s at %lu\n",
				measured_name,seq_no);
			fflush(stderr);
			free(matches);
			return(NULL);
		}
		next_ts = (double)ntohl(next_rv.tv_sec)+(double)(ntohl(next_rv.tv_usec) / 1000000.0);
		m_ts = (double)ntohl(m_rv.tv_sec)+(double)(ntohl(m_rv.tv_usec) / 1000000.0);
		while(next_ts < p_ts) {
			memcpy(&m_rv,&next_rv,sizeof(m_rv));
			seq_no++;
			if(seq_no > m_seq_no) {
				break;
			}
			err = WooFGet(measured_name,(void *)&next_rv,seq_no);
			if(err < 0) {
				fprintf(stderr,"couldn't get next loop rv from %s at %lu\n",
				measured_name,seq_no);
				fflush(stderr);
				free(matches);
				return(NULL);
			}
			next_ts = (double)ntohl(next_rv.tv_sec)+(double)(ntohl(next_rv.tv_usec) / 1000000.0);
			m_ts = (double)ntohl(m_rv.tv_sec)+(double)(ntohl(m_rv.tv_usec) / 1000000.0);
		}
		/*
		 * end case
		 */
		if((next_ts < p_ts) && (count == 0)) {
			matches[i*2+0] = p_rv.value.d;
			matches[i*2+1] = m_rv.value.d;
		}
			
	}

	return(matches);
			
}

/*
 * dispatches incoming data
 */
int RegressPairPredictedHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	REGRESSINDEX ri;
	REGRESSVAL result_rv;
	char index_name[4096+64];
	char measured_name[4096+64];
	char predicted_name[4096+64];
	char result_name[4096+64];
	int count_back;
	unsigned long seq_no;
	double *matches;
	double p;
	double m;
	int i;
	int err;
	struct timeval tm;

	MAKE_EXTENDED_NAME(index_name,rv->woof_name,"index");
	MAKE_EXTENDED_NAME(measured_name,rv->woof_name,"measured");
	MAKE_EXTENDED_NAME(predicted_name,rv->woof_name,"predicted");
	MAKE_EXTENDED_NAME(result_name,rv->woof_name,"result");


	memcpy(result_rv.woof_name,rv->woof_name,sizeof(result_rv.woof_name));

	/*
	 * get the count back from the index
	 */
	seq_no = WooFGetLatestSeqno(index_name); 

	err = WooFGet(index_name,(void *)&ri,seq_no);
	if(err < 0) {
		fprintf(stderr,
			"RegressPairPredictedHandler couldn't get count back from %s\n",index_name);
		return(-1);
	}

	matches = ComputeMatchces(predicted_name,measured_name,ri.count_back);

	if(matches == NULL) {
		fprintf(stderr,"couldn't compute matches from %s and %s\n",
			predicted_name,
			measured_name);
		fflush(stderr);
		return(-1);
	}

	for(i=0; i < count_back; i++) {
		p = matches[i*2+0];
		m = matches[i*2+1];
		gettimeofday(&tm,NULL);
		result_rv.tv_sec = htonl(tm.tv_sec);
		result_rv.tv_usec = htonl(tm.tv_usec);
		result_rv.type = 'd';
		result_rv.value.d = (p - m); /* diff for testing */
		seq_no = WooFPut(result_name,NULL,(void *)&result_rv);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,
				"RegressPairPredictedHandler: couldn't put result to %s\n",result_name);
			free(matches);
			return(-1);
		}
	}

	free(matches);

	return(1);
}

