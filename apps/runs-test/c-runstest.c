#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "c-twist.h"
#include "ks.h"
#include "normal.h"

#define DEBUG

// from https://en.wikipedia.org/wiki/Wald–Wolfowitz_runs_test
#ifndef HAS_WOOF
double RunsStat(double *v, int N)
{
	int i;
	double n_plus = 0;
	double n_minus = 0;
	double mu;
	double sigma_sq;
	double stat;
	double runs;
	int b1;
	int b2;

	if(N == 0) {
		return(-1);
	}

	for(i=0; i < N; i++) {
		if(v[i] >= 0.5) {
			n_plus++;
		} else {
			n_minus++;
		}
	}

	mu = ((2*n_plus*n_minus) / (double)N) + 1.0;
	sigma_sq = ((mu - 1) * (mu - 2)) / ((double)N-1.0);

	runs = 1;

	for(i=1; i < N; i++) {
		if(v[i-1] >= 0.5) {
			b1 = 1;
		} else {
			b1 = 0;
		}
		if(v[i] >= 0.5) {
			b2 = 1;
		} else {
			b2 = 0;
		}

#ifdef DEBUG
		if(b1 == 1) {
			printf("+");
		} else {
			printf("-");
		}
#endif

		if(b1 != b2) {
			runs++;
		}
	}


#ifdef DEBUG
	if(b2 == 1) {
		printf("+\n");
	} else {
		printf("-\n");
	}
	printf("runs: %f\n",runs);
	fflush(stdout);
#endif

	stat = (runs - mu) / sqrt(sigma_sq);

	return(stat);
}

#else

/*
 * WooF version
 */
#include "woofc.h"

double RunsStat(char *data_woof, int N)
{
	WOOF *wf;
	double v;
	double v2;
	int i;
	double n_plus = 0;
	double n_minus = 0;
	double mu;
	double sigma_sq;
	double stat;
	double runs;
	int b1;
	int b2;
	unsigned long latest;
	unsigned long earliest;
	unsigned long ndx;
	int err;
	

	if(N == 0) {
		return(-1);
	}

	wf = WooFOpen(data_woof);
	if(wf == NULL) {
		fprintf(stderr,"RunStat couldn't open %s\n",
			data_woof);
		fflush(stderr);
		exit(1);
	}


	i = 0;
	for(ndx=WooFLatest(wf); i < N; ndx = WooFBack(wf,i)) {
		err = WooFGet(wf,(void *)&v,ndx);
		if(err <= 0) {
			fprintf(stderr,
			"WooFGet couldn't get random values from %s at %lu\n",
			data_woof,ndx);
			fflush(stderr);
			exit(1);
		}

#ifdef DEBUG
		printf("data_woof: %s, ndx: %lu, v: %f\n",
			data_woof,ndx,v);
		fflush(stdout);
#endif
		if(v >= 0.5) {
			n_plus++;
		} else {
			n_minus++;
		}
		i++;
	}

	if(i != N) {
		fprintf(stderr,"short history: i: %d, N: %d\n",
			i,N);
		fflush(stderr);
		exit(1);
	}

	mu = ((2*n_plus*n_minus) / (double)N) + 1.0;
	sigma_sq = ((mu - 1) * (mu - 2)) / ((double)N-1.0);

#ifdef DEBUG
	printf("mu: %f, sigma_sq: %f\n",mu,sigma_sq);
	fflush(stdout);
#endif	

	runs = 1;
	ndx = WooFBack(wf,N-1);
	err = WooFGet(wf,(void *)&v,ndx);
	if(err <= 0) {
		fprintf(stderr,
		"WooFGet couldn't reget random values from %s at %lu\n",
			data_woof,ndx);
			fflush(stderr);
			exit(1);
	}

	ndx = WooFBack(wf,N-2);
	for(i=1; i < N; i++) {
		err = WooFGet(wf,(void *)&v2,ndx);
		if(err <= 0) {
			fprintf(stderr,
		"WooFGet couldn't reget second random values from %s at %lu\n",
			data_woof,ndx);
			fflush(stderr);
			exit(1);
		}
		if(v >= 0.5) {
			b1 = 1;
		} else {
			b1 = 0;
		}
		if(v2 >= 0.5) {
			b2 = 1;
		} else {
			b2 = 0;
		}

#ifdef DEBUG
		if(b1 == 1) {
			printf("+");
		} else {
			printf("-");
		}
#endif

		if(b1 != b2) {
			runs++;
		}
		v = v2;
		ndx = WooFNext(wf,ndx);
	}


#ifdef DEBUG
	if(b2 == 1) {
		printf("+\n");
	} else {
		printf("-\n");
	}
	printf("runs: %f\n",runs);
	fflush(stdout);
#endif

	stat = (runs - mu) / sqrt(sigma_sq);

	WooFFree(wf);

	return(stat);
}

#endif

#ifdef STANDALONE 

#define ARGS "a:c:S:s:"
char *Usage = "usage: c-runstest -c count (iterations)\n\
\t-a alpha-level (default 0.05)\n\
\t-s sample_size\n\
\t-S seed\n";

uint32_t Seed;

int main(int argc, char **argv)
{
	int c;
	int count;
	int sample_size;
	int i;
	int j;
	int has_seed;
	struct timeval tm;
	double *r;
	double value;
	double incr;
	double stat;
	double norm;
	double kstat;
	double alpha;
	double critical;
	void *d1;
	void *d2;
	

	count = 100;
	sample_size = 30;
	has_seed = 0;
	alpha = 0.05;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'a':
				alpha = atof(optarg);
				break;
			case 'c':
				count = atoi(optarg);
				break;
			case 's':
				sample_size = atoi(optarg);
				break;
			case 'S':
				Seed = atoi(optarg);
				has_seed = 1;
				break;
			default:
				fprintf(stderr,"uncognized command -%c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(count <= 0) {
		fprintf(stderr,"count must be non-negative\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(sample_size <= 0) {
		fprintf(stderr,"sample_size must be non-negative\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if((alpha <= 0.0) || (alpha >= 1.0)) {
		fprintf(stderr,"alpha must be between 0.0 and 1.0\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(has_seed == 0) {
		gettimeofday(&tm,NULL);
		Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	}

	r = (double *)malloc(sample_size * sizeof(double));
	if(r == NULL) {
		fprintf(stderr,"no space\n");
		exit(1);
	}

	CTwistInitialize(Seed);
	srand48(Seed);

	InitDataSet(&d1,1);
	InitDataSet(&d2,1);

	value = 0.000000001;
	incr = 1.0 / (double)count;
	for(j=0; j < count; j++) {
		for(i=0; i < sample_size; i++) {
			r[i] = CTwistRandom();
		}
		stat = RunsStat(r,sample_size);
		norm = InvNormal(value,0.0,1.0);
		WriteData(d1,1,&stat);
		WriteData(d2,1,&norm);
		value = value + incr;
		if(value >= 1.0) {
			value = 0.999999999999;
		}
	}

	critical = KSCritical(alpha,(double)count,(double)count);

	kstat = KS(d1,1,d2,1);

	printf("ks stat: %f alpha: %f critical value: %f\n",
		kstat,alpha,critical);
	fflush(stdout);

	FreeDataSet(d1);
	FreeDataSet(d2);

	return(0);
}

#endif
