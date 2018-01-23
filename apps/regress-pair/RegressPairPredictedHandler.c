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
#include "regress-matrix.h"

#include "woofc.h"

FILE *fd;


double *ComputeMatchces(char *predicted_name,char *measured_name,
		int count_back, int *r_count_back)
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

	if((int)(p_seq_no - count_back) <= 0) {
		fprintf(stderr,"not enough history for %d countback in %s, seq_no: %lu\n",
			count_back,predicted_name,p_seq_no);
		fflush(stderr);
		free(matches);
		return(NULL);
	}
	if((int)(m_seq_no - count_back) <= 0) {
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
	while((int)seq_no > 0) {
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
		if(i >= count_back) {
			break;
		}
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
			*r_count_back = i;
			return(matches);
		}
		next_ts = (double)ntohl(next_rv.tv_sec)+(double)(ntohl(next_rv.tv_usec) / 1000000.0);
		m_ts = (double)ntohl(m_rv.tv_sec)+(double)(ntohl(m_rv.tv_usec) / 1000000.0);
		while(next_ts < p_ts) {
			memcpy(&m_rv,&next_rv,sizeof(m_rv));
			seq_no++;
			if(seq_no > m_seq_no) {
				matches[i*2+0] = p_rv.value.d;
				matches[i*2+1] = next_rv.value.d;
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
	}

	*r_count_back = i;
	return(matches);
			
}

int RegressPairPredictedHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	REGRESSINDEX ri;
	REGRESSVAL p_rv;
	REGRESSCOEFF coeff_rv;
	char index_name[4096+64];
	char measured_name[4096+64];
	char predicted_name[4096+64];
	char coeff_name[4096+64];
	char result_name[4096+64];
	int count_back;
	unsigned long seq_no;
	double *matches;
	double p;
	double m;
	int i;
	int err;
	struct timeval tm;
	Array2D *match_array;
	double *coeff;

#ifdef DEBUG
        fd = fopen("/cspot/pred-handler.log","a+");
        fprintf(fd,"RegressPairPredictedHandler called on woof %s, type %c\n",
                rv->woof_name,rv->series_type);
        fflush(fd);
        fclose(fd);
#endif  

	MAKE_EXTENDED_NAME(index_name,rv->woof_name,"index");
	MAKE_EXTENDED_NAME(measured_name,rv->woof_name,"measured");
	MAKE_EXTENDED_NAME(predicted_name,rv->woof_name,"predicted");
	MAKE_EXTENDED_NAME(result_name,rv->woof_name,"result");
	MAKE_EXTENDED_NAME(coeff_name,rv->woof_name,"coeff");


	memcpy(coeff_rv.woof_name,rv->woof_name,sizeof(coeff_rv.woof_name));

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

	matches = ComputeMatchces(predicted_name,measured_name,ri.count_back,&count_back);

	if(matches == NULL) {
		fprintf(stderr,"couldn't compute matches from %s and %s\n",
			predicted_name,
			measured_name);
		fflush(stderr);
		return(-1);
	}

	match_array = MakeArray2D(2,count_back);
	if(match_array == NULL) {
		free(matches);
		return(-1);
	}

	for(i=0; i < count_back; i++) {
		match_array->data[i*2+0] = matches[i*2+0];
		match_array->data[i*2+1] = matches[i*2+1];
	}

	coeff = RegressMatrix(match_array);
	if(coeff == NULL) {
		fprintf(stderr,"RegressPairPredictedHandler: couldn't get regression coefficients\n");
		FreeArray2D(match_array);
		free(matches);
		return(-1);
	}
	FreeArray2D(match_array);
	free(matches);

	seq_no = WooFGetLatestSeqno(predicted_name);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"couldn't get latest seq_no from %s\n",
			predicted_name);
		fflush(stderr);
		free(coeff);
		return(-1);
	}

	err = WooFGet(predicted_name,&p_rv,seq_no);
	if(err < 0) {
		fprintf(stderr,"couldn't get latest record from %s\n",
			predicted_name);
		fflush(stderr);
		free(coeff);
		return(-1);
	}

	coeff_rv.tv_sec = p_rv.tv_sec;
	coeff_rv.tv_usec = p_rv.tv_usec;
	coeff_rv.intercept = coeff[0];
	coeff_rv.slope = coeff[1];
printf("SLOPE: %f int: %f\n",coeff_rv.slope,coeff_rv.intercept);
fflush(stdout);
	free(coeff);
	seq_no = WooFPut(coeff_name,NULL,(void *)&coeff_rv);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,
			"RegressPairPredictedHandler: couldn't put result to %s\n",result_name);
		return(-1);
	}

	return(1);
}

