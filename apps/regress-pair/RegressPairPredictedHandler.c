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

#include "redblack.h"
#include "regress-pair.h"
#include "regress-matrix.h"
#include "ssa-decomp.h"

#include "woofc.h"

FILE *fd;

#define SLEEPTIME (1)

#define MAXINTERVAL (1200.0)

Array2D *ComputeMatchArray(Array2D *pred_series, Array2D *meas_series)
{
	int i;
	int j;
	int err;
	int mcount;
	int k;
	double p_ts;
	double m_ts;
	double v_ts;
	double next_ts;
	double pv;
	double mv;
	Array2D *matched_array;

	matched_array = MakeArray2D(pred_series->ydim, 2);
	if (matched_array == NULL)
	{
		return (NULL);
	}

	i = 0;
	j = 0;
	k = 0;
	m_ts = meas_series->data[i * 2 + 0];
	next_ts = meas_series->data[(i + 1) * 2 + 0];
	p_ts = pred_series->data[j * 2 + 0];
	while (j < pred_series->ydim)
	{
		if (m_ts <= p_ts)
		{
			break;
		}
		j++;
		p_ts = pred_series->data[j * 2 + 0];
	}
	while (((i + 1) < meas_series->ydim) && (j < pred_series->ydim))
	{
		if (fabs(p_ts - next_ts) < fabs(p_ts - m_ts))
		{
			matched_array->data[k * 2 + 0] = pred_series->data[j * 2 + 1];
			matched_array->data[k * 2 + 1] = meas_series->data[(i + 1) * 2 + 1];
			v_ts = next_ts;
		}
		else
		{
			matched_array->data[k * 2 + 0] = pred_series->data[j * 2 + 1];
			matched_array->data[k * 2 + 1] = meas_series->data[i * 2 + 1];
			v_ts = m_ts;
		}

		if (fabs(v_ts - p_ts) > MAXINTERVAL)
		{
			printf("DROPPING(%d) match for pred ts %10.0f and meas ts %10.0f\n",
				   k, p_ts, v_ts);
			fflush(stdout);
			if (v_ts > p_ts)
			{ // move predictions forward
				while (v_ts > p_ts)
				{
					j++;
					if (j >= pred_series->ydim)
					{
						break;
					}
					p_ts = pred_series->data[j * 2 + 0];
				}
				if (j >= pred_series->ydim)
				{
					break;
				}
			}
			else
			{
				while (v_ts < p_ts)
				{ // move meas forward
					i++;
					if ((i + 1) >= meas_series->ydim)
					{
						break;
					}
					m_ts = next_ts;
					next_ts = meas_series->data[(i + 1) * 2 + 0];
					v_ts = next_ts;
				}
				if ((i + 1) >= meas_series->ydim)
				{
					break;
				}
			}
			continue; /* go back and try again */
		}
#ifdef DEBUG
		printf("MATCHED(%d): p: %10.10f %f m: %10.10f %f\n",
			   j, p_ts, matched_array->data[k * 2 + 0],
			   v_ts, matched_array->data[k * 2 + 1]);
		fflush(stdout);
#endif

		k++;
		i++;
		j++;
		if (j >= pred_series->ydim)
		{
			/*
printf("MATCHED SHORT: j: %d, i: %d, k: %d pydim: %lu mydim: %lu\n",
j,i,k,pred_series->ydim,meas_series->ydim);
fflush(stdout);
*/
			matched_array->ydim = k;
			return (matched_array);
		}
		if (pred_series->data[j * 2 + 0] < p_ts)
		{
			fprintf(stderr, "PANIC: pred series out of order p: %10.0f next: %10.0f\n",
					p_ts, pred_series->data[j * 2 + 0]);
			fflush(stderr);
			FreeArray2D(matched_array);
			return (NULL);
		}

		p_ts = pred_series->data[j * 2 + 0];
		m_ts = meas_series->data[i * 2 + 0];
		next_ts = meas_series->data[(i + 1) * 2 + 0];

		while (next_ts < p_ts)
		{
			m_ts = next_ts;
			i++;
			if ((i + 1) >= meas_series->ydim)
			{
				break;
			}
			next_ts = meas_series->data[(i + 1) * 2 + 0];
		}
		if ((i + 1) >= meas_series->ydim)
		{
			break;
		}
	}

	/*
printf("MATCHED END: j: %d, i: %d, k: %d, pydim: %lu mydim: %lu\n",
j,i,k,pred_series->ydim,meas_series->ydim);
fflush(stdout);
*/
	if (j < pred_series->ydim)
	{
		matched_array->data[k * 2 + 0] = pred_series->data[j * 2 + 1];
		matched_array->data[k * 2 + 1] = meas_series->data[i * 2 + 1];
	}
	matched_array->ydim = k;

	return (matched_array);
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
	for (i = 0; i < match_array->ydim; i++)
	{
		pred = (match_array->data[i * 2 + 1] * slope) + intercept;
		err = match_array->data[i * 2 + 0] - pred;
		sum += (err * err);
		count++;
	}

	if (count == 0)
	{
		return (-1);
	}

	*out_mse = sum / count;
	return (1);
}

Array2D *MakeArrayFromWooF(char *woof_name, unsigned long start_seq_no, unsigned long end_seq_no)
{
	unsigned long i;
	int size;
	int err;
	REGRESSVAL rv;
	Array2D *ar;

	size = (int)(end_seq_no - start_seq_no);

	ar = MakeArray2D(size, 2);
	if (ar == NULL)
	{
		return (NULL);
	}
	for (i = 0; i < size; i++)
	{
		err = WooFGet(woof_name, (void *)&rv, start_seq_no + i);
		if (err < 0)
		{
			fprintf(stderr, "MakeArrayFromWoof: err at seqno %lu\n", i);
			FreeArray2D(ar);
			return (NULL);
		}
		MAKETS(ar->data[i * 2 + 0], &rv);
		ar->data[i * 2 + 1] = rv.value.d;
	}

	return (ar);
}

int BestRegressionCoeff(char *predicted_name, unsigned long p_seq_no, char *measured_name, int count_back, int max_lags,
						double *out_slope, double *out_intercept, double *out_value, int *out_lags, unsigned long *out_seq_no)
{
	int i;
	int l;
	int err;
	unsigned long m_seq_no;
	unsigned long seq_no;
	int count;
	double p_ts;
	double m_ts;
	double now_ts;
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
	int best_lags;
	RB *sorted_meas = NULL;
	RB *sorted_pred = NULL;
	RB *rb;
	unsigned long earliest_seq_no;

	m_seq_no = WooFGetLatestSeqno(measured_name);
	if (WooFInvalid(m_seq_no))
	{
		fprintf(stderr, "BestCoeff: no latest seq no in %s\n",
				measured_name);
		goto out;
	}

	if ((int)(p_seq_no - count_back) <= 0)
	{
		fprintf(stderr, "BestCoeff: not enough history for %d countback in %s, seq_no: %lu\n",
				count_back, predicted_name, p_seq_no);
		goto out;
	}
	if ((int)(m_seq_no - count_back) <= 0)
	{
		fprintf(stderr, "BestCoeff: not enough history for %d countback in %s\n",
				count_back, measured_name);
		goto out;
	}

	/*
	 * get the current time stamp
	 */
	err = WooFGet(predicted_name, (void *)&p_rv, p_seq_no);
	if (err < 0)
	{
		fprintf(stderr, "BestCoeff: couldn't get now at %lu in %s\n",
				p_seq_no, predicted_name);
		goto out;
	}
	now_ts = (double)ntohl(p_rv.tv_sec) + (double)(ntohl(p_rv.tv_usec) / 1000000.0);

	/*
	 * get the earliest value from the predicted series
	 */
	count = count_back;
	err = WooFGet(predicted_name, (void *)&p_rv, (p_seq_no - count));
	if (err < 0)
	{
		fprintf(stderr, "BestCoeff: couldn't get earliest at %lu in %s\n",
				p_seq_no - count, predicted_name);
		goto out;
	}

	p_ts = (double)ntohl(p_rv.tv_sec) + (double)(ntohl(p_rv.tv_usec) / 1000000.0);
	/*
	 * walk back in the measured series and look for entry that is
	 * immediately before predicted value in terms of time stamp
	 */
	sorted_meas = RBInitD();
	if (sorted_meas == NULL)
	{
		fprintf(stderr, "BestCoeff: couldn't get space for sorted_meas\n");
		goto out;
	}
	seq_no = m_seq_no;
	measured_size = 0;
	while ((int)seq_no > 0)
	{
		err = WooFGet(measured_name, (void *)&m_rv, seq_no);
		if (err < 0)
		{
			fprintf(stderr, "BestCoeff: couldn't get measured at %lu in %s\n",
					seq_no, measured_name);
			goto out;
		}

		m_ts = (double)ntohl(m_rv.tv_sec) + (double)(ntohl(m_rv.tv_usec) / 1000000.0);
		/*
		 * if this measurement is too late, skip
		if(m_ts > now_ts) {
			seq_no--;
			continue;
		}
		 */
		if (m_ts < p_ts)
		{
			if (measured_size < count)
			{
				fprintf(stderr,
						"BestCoeff: MISMATCH: p: %lu %f m: %lu %f count: %d measured size: %d, p_seq_no: %lu, m_seq_no: %lu (%s)\n",
						ntohl(p_rv.tv_sec), p_rv.value.d, ntohl(m_rv.tv_sec), m_rv.value.d, count, measured_size, p_seq_no - count_back, seq_no, measured_name);
				fflush(stderr);
				/*
				goto out;
*/
			}
			break;
		}
		seq_no--;
		measured_size++;
		RBInsertD(sorted_meas, m_ts, m_rv.value);
	}

	if ((int)seq_no < 0)
	{
		fprintf(stderr, "BestCoeff: couldn't find ts earliesr than %10.0f in %s\n",
				p_ts, measured_name);
		goto out;
	}

	earliest_seq_no = seq_no;
	if (seq_no == 0)
	{
		earliest_seq_no = 1;
	}

	/*
	 * start with unsmoothed series
	 */
	unsmoothed = MakeArray2D(measured_size, 2);
	if (unsmoothed == NULL)
	{
		fprintf(stderr, "BestCoeff: no space for unsmoothed series size %d\n",
				measured_size);
		goto out;
	}

	pred_series = MakeArray2D(count_back, 2);
	if (pred_series == NULL)
	{
		fprintf(stderr, "BestCoeff: no space for pred series\n");
		goto out;
	}

	i = 0;
	RB_FORWARD(sorted_meas, rb)
	{
		m_ts = K_D(rb->key);
		unsmoothed->data[i * 2 + 0] = m_ts;
		unsmoothed->data[i * 2 + 1] = rb->value.d;
		i++;
	}

	sorted_pred = RBInitD();
	if (sorted_pred == NULL)
	{
		fprintf(stderr, "BestCoeff: no space for sorted predictions\n");
		goto out;
	}

	count = count_back;
	for (i = 0; i < pred_series->ydim; i++)
	{
		err = WooFGet(predicted_name, &p_rv, p_seq_no - count);
		if (err < 0)
		{
			fprintf(stderr, "BestCoeff: couldn't get predicted rv at %lu from %s\n",
					p_seq_no - count,
					predicted_name);
			goto out;
		}
		p_ts = (double)ntohl(p_rv.tv_sec) + (double)(ntohl(p_rv.tv_usec) / 1000000.0);
		RBInsertD(sorted_pred, p_ts, p_rv.value);
		count--;
	}

	i = 0;
	RB_FORWARD(sorted_pred, rb)
	{
		pred_series->data[i * 2 + 0] = K_D(rb->key);
		pred_series->data[i * 2 + 1] = rb->value.d;
		i++;
	}

	RBDestroyD(sorted_meas);
	sorted_meas = NULL;
	RBDestroyD(sorted_pred);
	sorted_pred = NULL;

	match_array = ComputeMatchArray(pred_series, unsmoothed);
	if (match_array == NULL)
	{
		fprintf(stderr, "BestCoeff: could get match array for %s %lu %s %lu\n",
				predicted_name, p_seq_no, measured_name, m_seq_no);
		goto out;
	}

#ifdef DEBUG
	printf("(%s) regressing... ", predicted_name);
	fflush(stdout);
#endif
	coeff = RegressMatrix(match_array);
	if (coeff == NULL)
	{
		fprintf(stderr, "BestCoeff: couldn't compute reg. coefficients from %s and %s\n",
				predicted_name, measured_name);
		goto out;
	}

	best_slope = coeff[1];
	best_intercept = coeff[0];
	best_lags = 0;
	i = match_array->ydim - 1;
	best_value = match_array->data[i * 2 + 1];
	err = MSE(best_slope, best_intercept, match_array, &mse);
	best_mse = mse;
	if (err < 0)
	{
		fprintf(stderr, "BestCoeff: couldn't get mse for unsmoothed regression\n");
		goto out;
	}
	free(coeff);
	coeff = NULL;
	FreeArray2D(match_array);
	match_array = NULL;

	for (l = 1; l <= max_lags; l++)
	{
		//	for(l = max_lags; l <= max_lags; l++) {
		smoothed = SSASmoothSeries(unsmoothed, l);
		if (smoothed == NULL)
		{
			fprintf(stderr, "BestCoeff: couldn't get smoothed array at lags %d\n",
					l);
			break;
		}
		match_array = ComputeMatchArray(pred_series, smoothed);
		if (match_array == NULL)
		{
			fprintf(stderr, "BestCoeff: couldn't get match at lags %d\n",
					l);
			break;
		}
		i = match_array->ydim - 1;
		last_value = match_array->data[i * 2 + 1];
		FreeArray2D(smoothed);
		smoothed = NULL;
		coeff = RegressMatrix(match_array);
		if (coeff == NULL)
		{
			fprintf(stderr, "BestCoeff: couldn't get regressin for lags %d\n",
					l);
			break;
		}
		err = MSE(coeff[1], coeff[0], match_array, &mse);
		if (err < 0)
		{
			fprintf(stderr, "BestCoeff: couldn't get mse for lags %d\n",
					l);
			break;
		}
		if (mse < best_mse)
		{
#ifdef DEBUG
			printf("(%s)better mse at lags %d: %f %f, best val %f\n",
				   predicted_name,
				   l, mse, best_mse, last_value);
			fflush(stdout);
#endif
			best_slope = coeff[1];
			best_intercept = coeff[0];
			best_mse = mse;
			best_value = last_value;
			best_lags = l;
		}
		free(coeff);
		coeff = NULL;
		FreeArray2D(match_array);
		match_array = NULL;
	}

	*out_slope = best_slope;
	*out_intercept = best_intercept;
	*out_value = best_value;
	*out_lags = best_lags;
	*out_seq_no = earliest_seq_no;
	if (coeff != NULL)
	{
		free(coeff);
	}
	if (match_array != NULL)
	{
		FreeArray2D(match_array);
	}
	if (unsmoothed != NULL)
	{
		FreeArray2D(unsmoothed);
	}
	if (smoothed != NULL)
	{
		FreeArray2D(smoothed);
	}
	FreeArray2D(pred_series);
	return (1);

out:
	if (unsmoothed != NULL)
	{
		FreeArray2D(unsmoothed);
	}
	if (smoothed != NULL)
	{
		FreeArray2D(smoothed);
	}
	if (pred_series != NULL)
	{
		FreeArray2D(pred_series);
	}
	if (match_array != NULL)
	{
		FreeArray2D(match_array);
	}
	if (coeff != NULL)
	{
		free(coeff);
	}
	if (sorted_meas != NULL)
	{
		RBDestroyD(sorted_meas);
	}
	if (sorted_pred != NULL)
	{
		RBDestroyD(sorted_pred);
	}
	fflush(stderr);
	return (-1);
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
	if (WooFInvalid(seq_no))
	{
		fprintf(stderr, "bad latest in next ready\n");
		return (-1);
	}

	next = 0;
	while (seq_no > 0)
	{
		err = WooFGet(request, (void *)&rv, seq_no);
		if (err < 0)
		{
			fprintf(stderr, "bad latest in next ready\n");
			return (-1);
		}

		/*
		 * is this a predicted request?
		 */
		if (rv.series_type != 'p')
		{
			seq_no--;
			continue;
		}

		/*
		 * yes, is it after the last finished?
		 */
		if (seq_no > last_finished)
		{
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
		if (next > 0)
		{
			rv.seq_no = next;
			*out_rv = rv;
			return (1);
		}
		else
		{
			return (0);
		}
	}

	return (0);
}

int HasDropOut(char *name, double start, double end, double interval)
{
	unsigned long seq_no;
	unsigned long l_seq_no;
	REGRESSVAL rv;
	double ts;
	double next_ts;
	int err;

	l_seq_no = seq_no = WooFGetLatestSeqno(name);
	if (WooFInvalid(seq_no))
	{
		printf("HASDROPOUT: bad last seqno\n");
		fflush(stdout);
		return (1);
	}
	if (seq_no == 0)
	{
		printf("HASDROPOUT: last seqno is 0\n");
		fflush(stdout);
		return (1);
	}

	err = WooFGet(name, &rv, seq_no);
	if (err < 0)
	{
		printf("HASDROPOUT: (%s) error getting seqno %lu\n", name);
		fflush(stdout);
		return (1);
	}
	ts = (double)ntohl(rv.tv_sec) + (double)(ntohl(rv.tv_usec) / 1000000.0);
	while (ts > start)
	{
		seq_no--;
		if (seq_no == 0)
		{
			printf("HASDROPOUT: (%s) reached seqno 0 for start %10.0f and ts %10.0f\n", name,
				   start, ts);
			fflush(stdout);
			return (1);
		}
		err = WooFGet(name, &rv, seq_no);
		if (err < 0)
		{
			printf("HASDROPOUT: (%s) error reading seqno %lu\n", name, seq_no);
			fflush(stdout);
			return (1);
		}
		ts = (double)ntohl(rv.tv_sec) + (double)(ntohl(rv.tv_usec) / 1000000.0);
	}

	seq_no++;
	next_ts = ts;
	while (next_ts < end)
	{
		err = WooFGet(name, &rv, seq_no);
		if (err < 0)
		{
			printf("HASDROPOUT: (%s) error 2 reading seqno %lu\n", name, seq_no);
			fflush(stdout);
			return (1);
		}
		next_ts = (double)ntohl(rv.tv_sec) + (double)(ntohl(rv.tv_usec) / 1000000.0);
		if (fabs(next_ts - ts) > interval)
		{
			printf("HASDROPOUT: (%s) %10.10f and %10.10f\n", name, next_ts, ts);
			fflush(stdout);

			return (1);
		}
		ts = next_ts;
		seq_no++;
		if (seq_no > l_seq_no)
		{
			return (0);
		}
	}

	return (0);
}

void FinishPredicted(char *finished_name, unsigned long wf_seq_no)
{
	unsigned long seq_no = wf_seq_no + 1;
	printf("PREDICTED (%s) finished %lu\n", finished_name, wf_seq_no);
	fflush(stdout);
	return;
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
	REGRESSCOEFF coeff_last_rv;
	char index_name[4096 + 64];
	char measured_name[4096 + 64];
	char predicted_name[4096 + 64];
	char coeff_name[4096 + 64];
	char result_name[4096 + 64];
	char error_name[4096 + 64];
	char finished_name[4096 + 64];
	char progress_name[4096 + 64];
	int count_back;
	unsigned long seq_no;
	unsigned long c_seq_no;
	unsigned long m_seq_no;
	unsigned long f_seq_no;
	unsigned long start_seq_no;
	unsigned long end_seq_no;
	unsigned long prev_seq_no;
	double p;
	double m;
	int i;
	int err;
	struct timeval tm;
	Array2D *match_array;
	double *coeff;
	double pred;
	double actual;
	double error;
	double p_ts;
	double m_ts;
	double e_ts;
	Array2D *pred_array;
	Array2D *s_array;
	double pred_time;
	double measure;
	RB *sorted_meas;
	RB *rb;
	int mcount;

#ifdef TIMING
	struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);
#endif

#ifdef DEBUG
	fd = fopen("/cspot/pred-handler.log", "a+");
	fprintf(fd, "RegressPairPredictedHandler called on woof %s, type %c\n",
			rv->woof_name, rv->series_type);
	fflush(fd);
	fclose(fd);
#endif

#ifdef DEBUG
	printf("PREDICTED (%s) START for %lu\n", rv->woof_name, wf_seq_no);
	fflush(stdout);

	printf("PREDICTED (%s) AWAKE for %lu\n", rv->woof_name, wf_seq_no);
	fflush(stdout);
#endif

	/*
	 * my turn
	 */
	MAKE_EXTENDED_NAME(index_name, rv->woof_name, "index");
	MAKE_EXTENDED_NAME(measured_name, rv->woof_name, "measured");
	MAKE_EXTENDED_NAME(predicted_name, rv->woof_name, "predicted");
	MAKE_EXTENDED_NAME(result_name, rv->woof_name, "result");
	MAKE_EXTENDED_NAME(error_name, rv->woof_name, "errors");
	MAKE_EXTENDED_NAME(coeff_name, rv->woof_name, "coeff");
	MAKE_EXTENDED_NAME(progress_name, rv->woof_name, "progress");
	sprintf(index_name, "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/test.index");
	sprintf(measured_name, "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test.measured");
	sprintf(predicted_name, "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/test.predicted");
	sprintf(result_name, "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/test.result");
	sprintf(error_name, "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/test.errors");
	sprintf(coeff_name, "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/test.coeff");

	/*
	 * start by computing the forecasting error if possible
	 */
	c_seq_no = WooFGetLatestSeqno(coeff_name);
	m_seq_no = WooFGetLatestSeqno(measured_name);

	MAKETS(p_ts, rv);
	/*
	 * get the count back from the index
	 */
	seq_no = WooFGetLatestSeqno(index_name);

	err = WooFGet(index_name, (void *)&ri, seq_no);
	if (err < 0)
	{
		fprintf(stderr,
				"RegressPairPredictedHandler couldn't get count back from %s at %lu\n", index_name, seq_no);
		FinishPredicted(finished_name, wf_seq_no);
		return (-1);
	}

	if ((int)(wf_seq_no - ri.count_back) <= 0)
	{
		fprintf(stderr, "not enough history at seq_no %lu\n", wf_seq_no);
		FinishPredicted(finished_name, wf_seq_no);
		return (-1);
	}

	err = WooFGet(predicted_name, &ev, (wf_seq_no - ri.count_back));
	if (err < 0)
	{
		fprintf(stderr, "couldn't get earliest ts in prtedicted series at %lu\n",
				wf_seq_no - ri.count_back);
		FinishPredicted(finished_name, wf_seq_no);
		return (-1);
	}

	MAKETS(e_ts, &ev);

	sorted_meas = RBInitD();
	if (sorted_meas == NULL)
	{
		fprintf(stderr, "RegressPairPredictedHandler no space for rb\n");
		fflush(stderr);
		FinishPredicted(finished_name, wf_seq_no);
		return (-1);
	}

	/*
	 * get the data from the measured woof for the time period between p_ts and e_ts
	 */
	if (m_seq_no > 0)
	{
		mcount = 0;
		err = WooFGet(measured_name, (void *)&mv, m_seq_no);
		while ((err > 0) && (m_seq_no > 0))
		{
			MAKETS(m_ts, &mv);
			if (m_ts > p_ts)
			{
				m_seq_no--;
				err = WooFGet(measured_name, (void *)&mv, m_seq_no);
				continue;
			}
			if (m_ts < e_ts)
			{
				break;
			}
			/*
			 * could be out of order
			 */
			RBInsertD(sorted_meas, m_ts, mv.value);
			mcount++;
			m_seq_no--;
			err = WooFGet(measured_name, (void *)&mv, m_seq_no);
		}
		if((mcount == 0) && (err < 0)) {
			if(err < 0) {
				fprintf(stderr,"couldn't find valid measurement for request %lu\n",
					rv->seq_no);
				RBDestroyD(sorted_meas);
				FinishPredicted(finished_name, wf_seq_no);
				return (-1);
			}
			if (m_seq_no == 0)
			{
				fprintf(stderr, "couldn't find meas ts earlier that %10.0f, using %10.0f\n",
						p_ts, m_ts);
				RBDestroyD(sorted_meas);
				FinishPredicted(finished_name,wf_seq_no);
			}
		}
	}

	if ((c_seq_no > 0) && (mcount > 0))
	{

		/*
		 * get the latest measurement data and put it in array for smoothing
		 */
		pred_array = MakeArray2D(mcount, 2);
		if (pred_array == NULL)
		{
			fprintf(stderr, "couldn't make array from %s from %lu to %lu\n",
					measured_name, start_seq_no, end_seq_no);
			FinishPredicted(finished_name, wf_seq_no);
			return (-1);
		}
		i = 0;
		RB_FORWARD(sorted_meas, rb)
		{
			pred_array->data[i * 2 + 0] = K_D(rb->key);
			pred_array->data[i * 2 + 1] = rb->value.d;
			i++;
		}
		RBDestroyD(sorted_meas);

		/*
		 * get the latest set of coefficients
		 */
		seq_no = WooFGet(coeff_name, (void *)&coeff_rv, c_seq_no);
		if (!WooFInvalid(seq_no))
		{
			//			pred = (coeff_rv.slope * mv.value.d) + coeff_rv.intercept;
			//			pred = (coeff_rv.slope * coeff_rv.measure) + coeff_rv.intercept;
			/*
			 * make a smoothed series using the latest measurement data
			 * with the same lags we used to produce the
			 * coeff
			 */
			if (coeff_rv.lags > 0)
			{
				s_array = SSASmoothSeries(pred_array, coeff_rv.lags);
				actual = pred_array->data[(pred_array->ydim - 1) * 2 + 1];
				FreeArray2D(pred_array);
				if (s_array == NULL)
				{
					fprintf(stderr, "couldn't create mooth array, lags %d form %lu to %lu\n",
							coeff_rv.lags, start_seq_no, end_seq_no);
					FinishPredicted(finished_name, wf_seq_no);
					return (-1);
				}
			}
			else
			{
				s_array = pred_array;
				actual = pred_array->data[(pred_array->ydim - 1) * 2 + 1];
			}
			measure = s_array->data[(s_array->ydim - 1) * 2 + 1];
			pred = (coeff_rv.slope * measure) + coeff_rv.intercept;
			pred_time = s_array->data[(s_array->ydim - 1) * 2 + 0];
			FreeArray2D(s_array);
			error = rv->value.d - pred;
			ev.value.d = error;
			ev.tv_sec = rv->tv_sec;
			ev.tv_usec = rv->tv_usec;
			memcpy(ev.woof_name, rv->woof_name, sizeof(ev.woof_name));
			seq_no = WooFPut(error_name, NULL, &ev);
			if (WooFInvalid(seq_no))
			{
				fprintf(stderr, "error seq_no %lu invalid on put\n", seq_no);
			}
			printf("ERROR: (%s) %lu predicted: %f prediction: %f measure: %10.10f %f, actual: %f error: %f slope: %f int: %f\n", rv->woof_name, ntohl(rv->tv_sec),
				   rv->value.d, pred, pred_time, measure, actual, error, coeff_rv.slope, coeff_rv.intercept);
			fflush(stdout);
		}
		else
		{
			fprintf(stderr, "coeff seq_no %lu invalid from %s\n", seq_no, coeff_name);
		}
	}

	/*
	 * now compute the coffe for the future using the most up to date ground truth
	 */
	memcpy(coeff_rv.woof_name, rv->woof_name, sizeof(coeff_rv.woof_name));

	err = BestRegressionCoeff(predicted_name, wf_seq_no, measured_name, ri.count_back, ri.max_lags,
							  &coeff_rv.slope, &coeff_rv.intercept,
							  &coeff_rv.measure, &coeff_rv.lags, &coeff_rv.earliest_seq_no);

	// if (err < 0)
	// {
	// 	fprintf(stderr, "couldn't get best regression coefficient\n");
	// 	FinishPredicted(finished_name, wf_seq_no);
	// 	return (-1);
	// }

	if (err < 0 || coeff_rv.intercept > 100 || coeff_rv.intercept < -100)
	{
		// fprintf(stderr, "invalid coeff\n");
		// FinishPredicted(finished_name, wf_seq_no);
		// return (-1);
		seq_no = WooFGet(coeff_name, (void *)&coeff_last_rv, c_seq_no);
		if (!WooFInvalid(seq_no))
		{
			coeff_rv.slope = coeff_last_rv.slope;
			coeff_rv.intercept = coeff_last_rv.intercept;
		}
	}

	coeff_rv.tv_sec = rv->tv_sec;
	coeff_rv.tv_usec = rv->tv_usec;
	seq_no = WooFPut(coeff_name, NULL, (void *)&coeff_rv);
	if (WooFInvalid(seq_no))
	{
		fprintf(stderr,
				"RegressPairPredictedHandler: couldn't put coeff to %s\n", coeff_name);
		FinishPredicted(finished_name, wf_seq_no);
		return (-1);
	}

	/*
	 * do this last to prevent request handler from firing before progress reflects new launch
	 */
	FinishPredicted(finished_name, wf_seq_no);
#ifdef TIMING
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("TIMING:RegressPairPredictedHandler: %f\n", elapsedTime);
	fflush(stdout);
#endif
	return (1);
}
