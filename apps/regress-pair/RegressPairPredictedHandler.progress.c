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
#include "ssa-decomp.h"

#include "woofc.h"

FILE *fd;


Array2D *ComputeMatchArray(Array2D *pred_series, Array2D *meas_series)
{
	int i;
	int j;
	int err;
	int mcount;
	double p_ts;
	double m_ts;
	double v_ts;
	double next_ts;
	double pv;
	double mv;
	Array2D *matched_array;

	matched_array = MakeArray2D(pred_series->ydim,2);
	if(matched_array == NULL) {
		return(NULL);
	}

	i = 0;
	j = 0;
	m_ts = meas_series->data[i*2+0];
	next_ts = meas_series->data[(i+1)*2+0];
	p_ts = pred_series->data[j*2+0];
	while((i+1) < meas_series->ydim) {
		if(fabs(p_ts-next_ts) < fabs(p_ts-m_ts)) {
			matched_array->data[j*2+0] = pred_series->data[j*2+1];
			matched_array->data[j*2+1] = meas_series->data[(i+1)*2+1];
			v_ts = next_ts;
		} else {
			matched_array->data[j*2+0] = pred_series->data[j*2+1];
			matched_array->data[j*2+1] = meas_series->data[i*2+1];
			v_ts = m_ts;
		}

printf("MATCHED(%d): p: %10.10f %f m: %10.10f %f\n",
j,p_ts,matched_array->data[j*2+0],
v_ts,matched_array->data[j*2+1]);
fflush(stdout);
		
		i++;
		j++;
		if(j >= pred_series->ydim) {
			return(matched_array);
		}
if(pred_series->data[j*2+0] < p_ts) {
fprintf(stderr,"PANIC: pred series out of order p: %10.0f next: %10.0f\n",
p_ts,pred_series->data[j*2+0]);
fflush(stderr);
FreeArray2D(matched_array);
return(NULL);
}

		p_ts = pred_series->data[j*2+0];
		m_ts = meas_series->data[i*2+0];
		next_ts = meas_series->data[(i+1)*2+0];
		
		while(next_ts < p_ts) {
			m_ts = next_ts;
			i++;
			if((i+1) >= meas_series->ydim) {
				break;
			}
			next_ts = meas_series->data[(i+1)*2+0];
		}
		if((i+1) >= meas_series->ydim) {
			break;
		}
	}
	matched_array->data[j*2+0] = pred_series->data[j*2+1];
	matched_array->data[j*2+1] = meas_series->data[i*2+1];


	return(matched_array);
			
}

int MSE(double slope, double intercept, Array2D *match_array, double *out_mse)
{
	int i;
	double sum;
	double err;
	double count;
	double pred;

	count = 0;
	sum = 0;
	for(i=0; i < match_array->ydim; i++) {
		pred = (match_array->data[i*2+1]*slope) + intercept;
		err = match_array->data[i*2+0] - pred;
		sum += (err * err);
		count++;
	}

	if(count == 0) {
		return(-1);
	}

	*out_mse = sum / count;
	return(1);
}

	
#define MAXLAGS (15)
	

int BestRegressionCoeff(char *predicted_name, char *measured_name, int count_back,
	double *out_slope, double *out_intercept, double *out_value)
{
	int i;
	int l;
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
	double v_ts;
	int measured_size;
	Array2D *unsmoothed = NULL;
	Array2D *smoothed = NULL;
	Array2D *pred_series = NULL;
	Array2D *match_array = NULL;
	double *coeff = NULL;
	double mse;
	double best_mse;
	double best_slope;
	double best_intercept;
	double best_value;
	double last_value;

	m_seq_no = WooFGetLatestSeqno(measured_name);
	if(WooFInvalid(m_seq_no)) {
		fprintf(stderr,"BestCoeff: no latest seq no in %s\n",
			measured_name);
		goto out;
	}
	p_seq_no = WooFGetLatestSeqno(predicted_name);
	if(WooFInvalid(p_seq_no)) {
		fprintf(stderr,"BestCoeff: no latest seq no in %s\n",
			predicted_name);
		goto out;
	}

	if((int)(p_seq_no - count_back) <= 0) {
		fprintf(stderr,"BestCoeff: not enough history for %d countback in %s, seq_no: %lu\n",
			count_back,predicted_name,p_seq_no);
		goto out;
	}
	if((int)(m_seq_no - count_back) <= 0) {
		fprintf(stderr,"BestCoeff: not enough history for %d countback in %s\n",
			count_back,measured_name);
		goto out;
	}

	/*
	 * get the earliest value from the predicted series
	 */
	count = count_back;
	err = WooFGet(predicted_name,(void *)&p_rv,(p_seq_no - count));
	if(err < 0) {
		fprintf(stderr,"BestCoeff: couldn't get earliest at %lu in %s\n",
			p_seq_no-count,predicted_name);
		goto out;
	}

	/*
	 * walk back in the measured series and look for entry that is
	 * immediately before predicted value in terms of time stamp
	 */
	seq_no = m_seq_no;
	measured_size = 0;
	while((int)seq_no > 0) {
		err = WooFGet(measured_name,(void *)&m_rv,seq_no);
		if(err < 0) {
			fprintf(stderr,"BestCoeff: couldn't get measured at %lu in %s\n",
				seq_no,measured_name);
			goto out;
		}

		p_ts = (double)ntohl(p_rv.tv_sec)+(double)(ntohl(p_rv.tv_usec) / 1000000.0);
		m_ts = (double)ntohl(m_rv.tv_sec)+(double)(ntohl(m_rv.tv_usec) / 1000000.0);
		if(m_ts < p_ts) {
			if(measured_size < count) {
				fprintf(stderr,
"BestCoeff: MISMATCH: p: %lu %f m: %lu %f count: %d measured size: %d\n",
ntohl(p_rv.tv_sec),p_rv.value.d,ntohl(m_rv.tv_sec),m_rv.value.d,count,measured_size);
				fflush(stderr);
				goto out;
			}
			break;
		}
		seq_no--;
		measured_size++;
	}

	if((int)seq_no < 0) {
		fprintf(stderr,"BestCoeff: couldn't find ts earliesr than %10.0f in %s\n",
			p_ts,measured_name);
		goto out;
	}

	/*
	 * start with unsmoothed series
	 */
	unsmoothed = MakeArray2D(measured_size,2);
	if(unsmoothed == NULL) {
		fprintf(stderr,"BestCoeff: no space for unsmoothed series size %d\n",
			measured_size);
		goto out;
	}

	pred_series = MakeArray2D(count_back,2);
	if(pred_series == NULL) {
		fprintf(stderr,"BestCoeff: no space for pred series\n");
		goto out;
	}


	if(seq_no == 0) {
		seq_no = 1;
	}
	for(i=0; i < unsmoothed->ydim; i++) {
		err = WooFGet(measured_name,(void *)&m_rv,seq_no);
		if(err < 0) {
			fprintf(stderr,"BestCoeff: couldn't get measured rv at %lu (%d) from %s\n",
				seq_no,i,
				measured_name);
			goto out;
		}
		m_ts = (double)ntohl(m_rv.tv_sec)+(double)(ntohl(m_rv.tv_usec) / 1000000.0);
		unsmoothed->data[i*2+0] = m_ts;
		unsmoothed->data[i*2+1] = m_rv.value.d;
		seq_no++;
	} 

	count = count_back;
	for(i=0; i < pred_series->ydim; i++) {
		err = WooFGet(predicted_name,&p_rv,p_seq_no-count);
		if(err < 0) {
			fprintf(stderr,"BestCoeff: couldn't get predicted rv at %lu from %s\n",
				p_seq_no-count,
				predicted_name);
			goto out;
		}
		p_ts = (double)ntohl(p_rv.tv_sec)+(double)(ntohl(p_rv.tv_usec) / 1000000.0);
		if(i > 0) {
			if(pred_series->data[(i-1)*2+0] > p_ts) {
fprintf(stderr,"PANIC: out of order predicted seq_no (p_seq_no: %lu) at %lu, prev: %10.0f curr: %10.0f\n",
p_seq_no-count,p_seq_no,pred_series->data[(i-1)*2+0],p_ts);
				goto out;
			}
		}
		pred_series->data[i*2+0] = p_ts;
		pred_series->data[i*2+1] = p_rv.value.d;
		count--;
	} 

	match_array = ComputeMatchArray(pred_series,unsmoothed);
	if(match_array == NULL) {
		fprintf(stderr,"BestCoeff: could get match array for %s %lu %s %lu\n",
			predicted_name,p_seq_no,measured_name,m_seq_no);
		goto out;
	}

	coeff = RegressMatrix(match_array);
	if(coeff == NULL) {
		fprintf(stderr,"BestCoeff: couldn't compute reg. coefficients from %s and %s\n",
			predicted_name,measured_name);
		goto out;
	}

	best_slope = coeff[1];
	best_intercept = coeff[0];
	i = match_array->ydim - 1;
	best_value = match_array->data[i*2+1];
	err = MSE(best_slope,best_intercept,match_array,&mse);
	best_mse = mse;
	if(err < 0) {
		fprintf(stderr,"BestCoeff: couldn't get mse for unsmoothed regression\n");
		goto out;
	}
	free(coeff);
	coeff=NULL;
	FreeArray2D(match_array);
	match_array = NULL;

	for(l = 1; l <= MAXLAGS; l++) {
		smoothed = SSASmoothSeries(unsmoothed,l);
		if(smoothed == NULL) {
			fprintf(stderr,"BestCoeff: couldn't get smoothed array at lags %d\n",
				l);
			break;
		}
		match_array = ComputeMatchArray(pred_series,smoothed);
		if(match_array == NULL) {
			fprintf(stderr,"BestCoeff: couldn't get match at lags %d\n",
					l);
			break;
		}
		i = match_array->ydim - 1;
		last_value = match_array->data[i*2+1];
		FreeArray2D(smoothed);
		smoothed = NULL;
		coeff = RegressMatrix(match_array);
		if(coeff == NULL) {
			fprintf(stderr,"BestCoeff: couldn't get regressin for lags %d\n",
				l);
			break;
		}
		err = MSE(coeff[1],coeff[0],match_array,&mse);
		if(err < 0) {
			fprintf(stderr,"BestCoeff: couldn't get mse for lags %d\n",
				l);
			break;
		}
		if(mse < best_mse) {
printf("better mse at lags %d: %f %f, best val %f\n",
l,mse,best_mse,last_value);
fflush(stdout);
			best_slope = coeff[1];
			best_intercept = coeff[0];
			best_mse = mse;
			best_value = last_value;
		}
		free(coeff);
		coeff = NULL;
		FreeArray2D(match_array);
		match_array = NULL;
		
	}
		

	*out_slope = best_slope;
	*out_intercept = best_intercept;
	*out_value = best_value;
	if(coeff != NULL) {
		free(coeff);
	}
	if(match_array != NULL) {
		FreeArray2D(match_array);
	}
	if(unsmoothed != NULL) {
		FreeArray2D(unsmoothed);
	}
	if(smoothed != NULL) {
		FreeArray2D(smoothed);
	}
	FreeArray2D(pred_series);
	return(1);


out:
	if(unsmoothed != NULL) {
		FreeArray2D(unsmoothed);
	}
	if(smoothed != NULL) {
		FreeArray2D(smoothed);
	}
	if(pred_series != NULL) {
		FreeArray2D(pred_series);
	}
	if(match_array != NULL) {
		FreeArray2D(match_array);
	}
	if(coeff != NULL) {
		free(coeff);
	}
	fflush(stderr);
	return(-1);
}

int NextReadyPrediction(char *request, unsigned long last_finished, REGRESSVAL *out_rv)
{
	unsigned long seq_no;
	unsigned long next; // next one after last_finished
	REGRESSVAL rv;
	int err;

	/*
	 * find the prediction request that is next after the last one finished
	 */
	seq_no = WooFGetLatestSeqno(request);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"bad latest in next ready\n");
		return(-1);
	}

	next = 0;
	while(seq_no > 0) {
		err = WooFGet(request,(void *)&rv,seq_no);
		if(err < 0) {
			fprintf(stderr,"bad latest in next ready\n");
				return(-1);
		}

		/*
		 * is this a predicted request?
		 */
		if(rv.series_type != 'p') {
			seq_no--;
			continue;
		}

		/*
		 * yes, is it after the last finished?
		 */
		if(seq_no > last_finished) {
			/*
			 * remember it
			 */
			next = seq_no;
			seq_no--;
			continue;
		}
		/*
		 * not after last finished.  If there was one before it, use it
		 */
		if(next > 0) {
			rv.seq_no = next;
			*out_rv = rv;
			return(1);
		} else {
			return(0);
		}
	}

	return(0);
}
		




int RegressPairPredictedHandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	REGRESSVAL *rv = (REGRESSVAL *)ptr;
	REGRESSINDEX ri;
	REGRESSVAL p_rv;
	REGRESSVAL next_rv;
	REGRESSVAL ev;
	REGRESSVAL mv;
	REGRESSCOEFF coeff_rv;
	char index_name[4096+64];
	char measured_name[4096+64];
	char predicted_name[4096+64];
	char coeff_name[4096+64];
	char result_name[4096+64];
	char error_name[4096+64];
	char finished_name[4096+64];
	char progress_name[4096+64];
	int count_back;
	unsigned long seq_no;
	unsigned long c_seq_no;
	unsigned long m_seq_no;
	unsigned long p_seq_no;
	double p;
	double m;
	int i;
	int err;
	struct timeval tm;
	Array2D *match_array;
	double *coeff;
	double pred;
	double error;
	double p_ts;
	double m_ts;

#ifdef DEBUG
        fd = fopen("/cspot/pred-handler.log","a+");
        fprintf(fd,"RegressPairPredictedHandler called on woof %s, type %c\n",
                rv->woof_name,rv->series_type);
        fflush(fd);
        fclose(fd);
#endif  

	/*
	 * poll for one before me to finish
	 */
	MAKE_EXTENDED_NAME(finished_name,rv->woof_name,"finished");
	while(1) {
		seq_no = WooFGetLatestSeqno(finished_name);
		if(WooFInvalid(seq_no) || (seq_no == 0)) {
			sleep(sleeptime);
			continue;
		}
		seq_no = WooFGet(finished_name,(void *)&f_seq_no);
		if(WooFInvalid(seq_no) || (seq_no == 0)) {
			sleep(SLEEPTIME);
			continue;
		}
		if((f_seq_no+1) < wf_seq_no) {
			sleep(SLEEPTIME);
			continue;
		}
	}
	seq_no = WooFGet(finished_name,&f_seq_no,seq_no);

	MAKE_EXTENDED_NAME(index_name,rv->woof_name,"index");
	MAKE_EXTENDED_NAME(measured_name,rv->woof_name,"measured");
	MAKE_EXTENDED_NAME(predicted_name,rv->woof_name,"predicted");
	MAKE_EXTENDED_NAME(result_name,rv->woof_name,"result");
	MAKE_EXTENDED_NAME(error_name,rv->woof_name,"errors");
	MAKE_EXTENDED_NAME(coeff_name,rv->woof_name,"coeff");
	MAKE_EXTENDED_NAME(progress_name,rv->woof_name,"progress");

	/*
	 * start by computing the forecasting error if possible
	 */
	c_seq_no = WooFGetLatestSeqno(coeff_name);
	m_seq_no = WooFGetLatestSeqno(measured_name);

	p_ts = (double)ntohl(rv->tv_sec)+(double)(ntohl(rv->tv_usec) / 1000000.0);
	if((c_seq_no > 0) && (m_seq_no > 0)) {

		/*
		 * walk back to find the last measurement immediately before the
		 * predicted value
		 */
		err = WooFGet(measured_name,(void *)&mv,m_seq_no);
		while((err > 0) && (m_seq_no > 0)) {
			m_ts = (double)ntohl(mv.tv_sec)+(double)(ntohl(mv.tv_usec) / 1000000.0);
			if(m_ts < p_ts) {
				break;
			}
			m_seq_no--;
			err = WooFGet(measured_name,(void *)&mv,m_seq_no);
		}

		if(err < 0) {
			fprintf(stderr,"couldn't find valid measurement for request %lu\n",
				rv->seq_no);
			seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
			return(-1);
		}
		if(m_seq_no == 0) {
			fprintf(stderr,"couldn't find meas ts earlier that %10.0f, using %10.0f\n",
					p_ts,m_ts);
		}
			
			
			
		/*
		 * get the latest set of coefficients
		 */
		seq_no = WooFGet(coeff_name,(void *)&coeff_rv,c_seq_no);
		if(!WooFInvalid(seq_no)) {
//			pred = (coeff_rv.slope * mv.value.d) + coeff_rv.intercept;
			pred = (coeff_rv.slope * coeff_rv.measure) + coeff_rv.intercept;
			error = rv->value.d - pred;
			ev.value.d = error;
			ev.tv_sec = rv->tv_sec;
			ev.tv_usec = rv->tv_usec;
			memcpy(ev.woof_name,rv->woof_name,sizeof(ev.woof_name));
			seq_no = WooFPut(error_name,NULL,&ev);
			if(WooFInvalid(seq_no)) {
				fprintf(stderr,"error seq_no %lu invalid on put\n", seq_no);
			}
printf("ERROR: %lu predicted: %f prediction: %f meas: %f error: %f slope: %f int: %f\n", ntohl(rv->tv_sec),
rv->value.d,pred,coeff_rv.measure,error,coeff_rv.slope,coeff_rv.intercept);
fflush(stdout);
		} else {
				fprintf(stderr,"coeff seq_no %lu invalid from %s\n", seq_no,coeff_name);
		}
	}


	/*
	 * get the count back from the index
	 */
	seq_no = WooFGetLatestSeqno(index_name); 

	err = WooFGet(index_name,(void *)&ri,seq_no);
	if(err < 0) {
		fprintf(stderr,
			"RegressPairPredictedHandler couldn't get count back from %s\n",index_name);
		seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
		return(-1);
	}
	memcpy(coeff_rv.woof_name,rv->woof_name,sizeof(coeff_rv.woof_name));

	seq_no = WooFGetLatestSeqno(predicted_name);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"couldn't get latest seq_no from %s\n",
			predicted_name);
		fflush(stderr);
		seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
		return(-1);
	}

	err = WooFGet(predicted_name,&p_rv,seq_no);
	if(err < 0) {
		fprintf(stderr,"couldn't get latest record from %s\n",
			predicted_name);
		fflush(stderr);
		seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
		return(-1);
	}

	err = BestRegressionCoeff(predicted_name,measured_name,ri.count_back,
		&coeff_rv.slope,&coeff_rv.intercept,&coeff_rv.measure);

	if(err < 0) {
		fprintf(stderr,"couldn't get best regression coefficient\n");
		seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
		return(-1);
	}

	coeff_rv.tv_sec = p_rv.tv_sec;
	coeff_rv.tv_usec = p_rv.tv_usec;
printf("SLOPE: %f int: %f\n",coeff_rv.slope,coeff_rv.intercept);
fflush(stdout);
	seq_no = WooFPut(coeff_name,NULL,(void *)&coeff_rv);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,
			"RegressPairPredictedHandler: couldn't put result to %s\n",result_name);
		seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
		return(-1);
	}

	/*
	 * get the sequence number for the last handler that is in progress
	 */
	seq_no = WooFGetLatestSeqno(progress_name);
	err = WooFGet(progress_name,&p_seq_no,seq_no);
	if(err < 0) {
		fprintf(stderr,
			"RegressPairPredictedHandler: couldn't get progress seq_no %s\n",progress_name);
	}
	/*
	 * if it is this handler, try and get the next one from the request list
	 */
	if(p_seq_no <= rv->seq_no) {
		/*
		 * get the next one
		 */
		err = NextReadyPrediction(rv->woof_name,rv->seq_no,&next_rv);
		/*
		 * if there is a next one, add it to the progress list and put it to the predicted WooF
		 */
		if(err > 0) {
			seq_no = WooFPut(progress_name,NULL,&next_rv.seq_no);
			if(!WooFInvalid(seq_no)) {
				seq_no = WooFPut(predicted_name,"RegressPairPredictedHandler",&next_rv);
				if(WooFInvalid(seq_no)) {
					fprintf(stderr,"bad seq_no on continuing put\n");
					return(-1);
				}
			} else {
				fprintf(stderr,"bad seq_no on continuing progress put\n");
				return(-1);
			}
		} else {
			fprintf(stderr,"err on get for last finished seq_no %lu, err: %d\n",rv->seq_no,err);
		}
	}

	/*
	 * do this last to prevent request handler from firing before progress reflects new launch
	 */
	seq_no = WooFPut(finished_name,NULL,(void *)&rv->seq_no);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,
			"RegressPairPredictedHandler: couldn't put finished to %s\n",finished_name);
	}
	return(1);
}
