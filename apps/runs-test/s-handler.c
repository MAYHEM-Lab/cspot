#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

#include "c-twist.h"
#include "c-runstest.h"
#include "normal.h"
#include "ks.h"

#include "woofc.h"
#include "cspot-runstat.h"

int SHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	FA *fa = (FA *)ptr;
	FA next_k;
	double stat;
	int err;

	/*
	 * sanity check
	 */
	if(fa->i >= fa->count) {
		pthread_exit(NULL);
	}

	/*
	 * sanity check
	 */
	if(fa->j != (fa->sample_size - 1)) {
		pthread_exit(NULL);
	}

	stat = RunsStat(fa->r,fa->sample_size);

	/*
	 * put the stat without a handler
	 */
	err = WooFPut(fa->stats,NULL,&stat);
	if(err < 0) {
		fprintf(stderr,"SHandler couldn't put stat\n");
		exit(1);
	}

#if 0
	if(ta->logfile != NULL) {
		fd = fopen(ta->logfile,"a");
	} else {
		fd = stdout;
	}
	fprintf(fd,"i: %d stat: %f\n",ta->i,stat);
	if(ta->logfile != NULL) {
		fclose(fd);
	} else {
		fflush(stdout);
	}
#endif


	if(fa->i == (fa->count - 1)) {
		memcpy(&next_k,fa,sizeof(FA));
		err = WooFPut("Kargs","KHandler",&next_k);
		if(err < 0) {
			fprintf(stderr,"SHandler: couldn't create KHandler\n");
			exit(1);
		}
	}
	pthread_exit(NULL);
}


void *RThread(void *arg)
{
	TA *ta = (TA *)arg;
	TA *next_r;
	TA *next_s;
	pthread_t tid;
	int err;

	if(ta->i >= ta->count) {
		free(ta);
		pthread_exit(NULL);
	}

	if(ta->j == 0) {
		ta->r = (double *)malloc(sizeof(double)*ta->sample_size);
		if(ta->r == NULL) {
			perror("Rthread: no space for r\n");
			exit(1);
		}
	} else if(ta->j >= ta->sample_size) {
		free(ta);
		pthread_exit(NULL);
	}

	/*
	 * CTwistRandom not thread safe
	 */
	sem_wait(ta->mutex);
	ta->r[ta->j] = CTwistRandom();
	sem_post(ta->mutex);

	next_r = (TA *)malloc(sizeof(TA));
	if(next_r == NULL) {
		exit(1);
	}

	memcpy(next_r,ta,sizeof(TA));
	/*
	 * if the buffer is full, launch next Rthread on new iteration
	 */
	if(ta->j == (ta->sample_size - 1)) {
		next_r->i = ta->i + 1;
		next_r->j = 0;
	} else {
		next_r->j = ta->j + 1;
	}

	/*
	 * generate next random number
	 */
	err = pthread_create(&tid,NULL,RThread,(void *)next_r);
	if(err < 0) {
		perror("Rthread launching SThread");
		exit(1);
	}
	pthread_detach(tid);

	/*
	 * if the buffer is full, create an SThread
	 */
	if(ta->j == (ta->sample_size - 1)) {
		next_s = (TA *)malloc(sizeof(TA));
		if(next_s == NULL) {
			exit(1);
		}
		memcpy(next_s,ta,sizeof(TA));
		err = pthread_create(&tid,NULL,SThread,(void *)next_s);
		if(err < 0) {
			perror("Rthread launching SThread");
			exit(1);
		}
		pthread_detach(tid);
	}

	free(ta);

	pthread_exit(NULL);
}


char LogFile[2048];

int main(int argc, char **argv)
{
	int c;
	int count;
	int sample_size;
	int i;
	int has_seed;
	struct timeval tm;
	double *r;
	double alpha;
	pthread_t tid;
	int err;
	TA *ta;

	count = 100;
	sample_size = 100;
	has_seed = 0;
	alpha = 0.05;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'a':
				alpha = atof(optarg);
				break;
			case 'L':
				strncpy(LogFile,optarg,sizeof(LogFile));
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
		fprintf(stderr,"alpha must be between 0 and 1\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}
		
	if(has_seed == 0) {
		gettimeofday(&tm,NULL);
		Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	}

	r = (double *)malloc(count * sizeof(double));
	if(r == NULL) {
		fprintf(stderr,"no space for r in main\n");
		exit(1);
	}

	CTwistInitialize(Seed);

	ta = (TA *)malloc(sizeof(TA));
	if(ta == NULL) {
		fprintf(stderr,"no space for ta in main\n");
		exit(1);
	}
	
	ta->mutex = (sem_t *)malloc(sizeof(sem_t));
	if(ta->mutex == NULL) {
		fprintf(stderr,"no space for mutex in main\n");
		exit(1);
	}
	sem_init(ta->mutex,1,1);

	ta->i = 0;
	ta->j = 0;
	ta->count = count;
	ta->sample_size = count;
	ta->r = NULL; /* will be malloed by RThread */
	ta->alpha = alpha;

	InitDataSet(&ta->stats,1);	/* for Runs stats */

	if(LogFile[0] != 0) {
		ta->logfile = LogFile;
	} else {
		ta->logfile = NULL;
	}
	err = pthread_create(&tid,NULL,RThread,(void *)ta);
	if(err < 0) {
		perror("main creating Rthread");
		exit(1);
	}
	pthread_detach(tid);

	pthread_exit(NULL);


	return(0);
}

