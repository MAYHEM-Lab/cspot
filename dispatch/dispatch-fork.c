#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

#include "sema.h"

pthread_mutex_t CLock;
int Counter;
int Finished;
int Total;
sema Throttle;

char Handler[255];

struct arg_stc {
	int tid;
	char handler[255];
};
typedef struct arg_stc TARG;

struct arg_ret_stc {
	double elapsed;
	int count;
};
typedef struct arg_ret_stc TRET;

double Elapsed(struct timeval start, struct timeval end)
{
	double elapsed;

	elapsed = ((double)end.tv_sec + (((double)end.tv_usec)/1000000.0)) -
	          ((double)start.tv_sec + (((double)start.tv_usec)/1000000.0));

	return(elapsed);
}

void *ForkerThread(void *arg)
{
	TARG *ta = (TARG *)arg;
	char *argv[3];
	int pid;
	int status;
	int my_count;
	char buffer[255];
	
	
	argv[0] = ta->handler;
	argv[2] = NULL;
	while(1) {
		pthread_mutex_lock(&CLock);
		/*
		 * finihsed counts completions
		 */
		if(Finished == Total) {
			pthread_mutex_unlock(&CLock);
			pthread_exit(NULL);
		/*
		 * counter counts starts
		 */
		} else if(Counter == 0) {
			pthread_mutex_unlock(&CLock);
			while(waitpid(-1, &status, 0) > 0) {
				pthread_mutex_lock(&CLock);
				Finished++;
				pthread_mutex_unlock(&CLock);
				V(&Throttle);
			}
			continue;
		}
		my_count = Counter;
		Counter--;
		pthread_mutex_unlock(&CLock);
		P(&Throttle);

		sprintf(buffer,"%d",my_count);
		argv[1] = buffer;

		pid = fork();
		if(pid == 0) {
			execve(ta->handler,argv,NULL);
			/*
			 * not reached
			 */
			perror("ForkerThread");
			pthread_exit(NULL);
		} else if (pid > 0) {
			while(waitpid(-1, &status, 0) > 0) {
				pthread_mutex_lock(&CLock);
				Finished++;
				pthread_mutex_unlock(&CLock);
				V(&Throttle);
			}
		} else {
			perror("ForkerThread");
		}
	}
}

#define ARGS "H:c:t:T:"
char *Usage="dispatch-fork -H handler -c count -t threads -T throttle\n";

int main(int argc, char **argv)
{
	int c;
	int threads = 0;
	pthread_t *tids;
	TARG *tas;
	struct timeval start;
	struct timeval end;
	double elapsed;
	int i;
	int err;
	int throttle;

	while((c=getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'H':
				strcpy(Handler,optarg);
				break;
			case 'c':
				Total = atoi(optarg);
				break;
			case 't':
				threads = atoi(optarg);
				break;
			case 'T':
				throttle = atoi(optarg);
				break;
			default:
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Handler[0] == 0) {
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(Total == 0) {
		Total = 1;
	}

	if(threads == 0) {
		threads = 1;
	}

	if(throttle == 0) {
		throttle = 1;
	}

	tids = (pthread_t *)malloc(threads*sizeof(pthread_t));
	tas = (TARG *)malloc(threads*sizeof(TARG));
	memset(tas,0,threads*sizeof(TARG));

	pthread_mutex_init(&CLock,NULL);
	InitSem(&Throttle,throttle);

	Counter = Total;

	gettimeofday(&start,NULL);
	for(i=0; i < threads; i++) {
		strcpy(tas[i].handler,Handler);
		err = pthread_create(&tids[i],NULL,ForkerThread,&tas[i]);
		if(err < 0) {
			perror("main: thread create");
			exit(1);
		}
	}
	for(i=0; i < threads; i++) {
		pthread_join(tids[i],NULL);
	}
	gettimeofday(&end,NULL);
	elapsed = Elapsed(start,end);

	printf("%d handlers in %f seconds is %f handlers/s\n",
		Total,
		elapsed,
		(double)Total/elapsed);
	fflush(stdout);
	exit(0);
}
	
	

