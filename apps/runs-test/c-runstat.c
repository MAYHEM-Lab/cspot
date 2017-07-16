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


#define ARGS "c:S:L:"
char *Usage = "usage: c-runstat -L logfile\n\
\t-c count\n\
\t-S seed\n";

uint32_t Seed;

struct thread_arg_stc
{
	sem_t *mutex;
	int i;
	int N;
	double *r;
	char *logfile;
};

typedef struct thread_arg_stc TA;

void *SThread(void *arg)
{
	TA *ta = (TA *)arg;
	FILE *fd;
	double stat;


	if(ta->i <= 5) {
		free(ta);
		pthread_exit(NULL);
	}

	stat = RunsStat(ta->r,(ta->i)-1);

	if(ta->logfile != NULL) {
		fd = fopen(ta->logfile,"a");
	} else {
		fd = stdout;
	}
	fprintf(fd,"i: %d, stat: %f\n",ta->i,stat);
	if(ta->logfile != NULL) {
		fclose(fd);
	} else {
		fflush(stdout);
	}

	if(ta->i >= ta->N) {
		sem_destroy(ta->mutex);
		free(ta->mutex);
		free(ta->r);
	}
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


	if(ta->i == ta->N) {
		free(ta);
		pthread_exit(NULL);
	}

	ta->r[ta->i] = CTwistRandom();
	sem_post(ta->mutex);

	next_r = (TA *)malloc(sizeof(TA));
	if(next_r == NULL) {
		exit(1);
	}
	next_r->i = ta->i + 1;
	next_r->N = ta->N;
	next_r->mutex = ta->mutex;
	next_r->r = ta->r;
	next_r->logfile = ta->logfile;

	err = pthread_create(&tid,NULL,RThread,(void *)next_r);
	if(err < 0) {
		perror("Rthread launching RThread");
		exit(1);
	}
	pthread_detach(tid);

	next_s = (TA *)malloc(sizeof(TA));
	if(next_s == NULL) {
		exit(1);
	}
	next_s->i = ta->i;
	next_s->N = ta->N;
	next_s->mutex = ta->mutex;
	next_s->r = ta->r;
	next_s->logfile = ta->logfile;

	err = pthread_create(&tid,NULL,SThread,(void *)next_s);
	if(err < 0) {
		perror("Rthread launching SThread");
		exit(1);
	}
	pthread_detach(tid);

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

