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


#define ARGS "c:S:s:L:"
char *Usage = "usage: c-runstat -L logfile\n\
\t-c count\n\
\t-s sample_size\n\
\t-S seed\n";

uint32_t Seed;

struct thread_arg_stc
{
	sem_t *mutex;
	int i;
	int j;
	int count;
	int sample_size;
	double *r;
	char *logfile;
};

typedef struct thread_arg_stc TA;

void *SThread(void *arg)
{
	TA *ta = (TA *)arg;
	TA *next_r;
	FILE *fd;
	double stat;
	pthread_t tid;
	int err;

	if(ta->i >= ta->count) {
		sem_destroy(ta->mutex);
		free(ta->mutex);
		free(ta->r);
		free(ta);
		pthread_exit(NULL);
	}

	next_r = (TA *)malloc(sizeof(TA));
	if(next_r == NULL) {
		perror("SThread no space\n");
		exit(1);
	}

	memcpy(next_r,ta,sizeof(TA));

	if(ta->j < ta->sample_size) {
		next_r->j = ta->j + 1;
	} else {
		stat = RunsStat(ta->r,(ta->sample_size)-1);
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
		next_r->i = ta->i + 1;
		next_r->j = 0;
		free(ta->r);
	}

	err = pthread_create(&tid,NULL,Rthread,(void *)next_r);
	if(err < 0) {
		perror("SThread launching Rthread\n");
		exit(1);
	}
	pthread_detach(tid);
	free(ta);

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
	next_r->j = ta->j + 1;
	err = pthread_create(&tid,NULL,RThread,(void *)next_r);
	if(err < 0) {
		perror("Rthread launching SThread");
		exit(1);
	}
	pthread_detach(tid);

	if((ta->j-1) == ta->sample_size) {
		next_s = (TA *)malloc(sizeof(TA));
		if(next_s == NULL) {
			exit(1);
		}
		memcpy(next_s,ta,sizeof(TA));
		err = pthread_create(&tid,NULL,SThread,(void *)next_r);
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
	int i;
	int has_seed;
	struct timeval tm;
	double *r;
	pthread_t tid;
	int err;
	TA *ta;

	count = 10;
	has_seed = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'L':
				strncpy(LogFile,optarg,sizeof(LogFile));
				break;
			case 'c':
				count = atoi(optarg);
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
	ta->N = count;
	ta->r = r;
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

	return(0);
}

