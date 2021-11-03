#define _GNU_SOURCE
#include <stdlib.h>
#include <fcntl.h>
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
	int pid;
	int status;
	int my_count;
	int pd[2];
	int cd[2];
	int err;
	char *fargv[2];
	char c;
	char p1[255];
	char p2[10];
	
	
	/*
	 * create forker process connected via a pipe
	 * pd is parent writing to child
	 * cd is child writing to parent
	 */
	err = pipe2(pd,O_DIRECT);
	if(err < 0) {
		fprintf(stderr,"parent pipe\n");
		perror("ForkerThread");
		pthread_exit(NULL);
	}
	err = pipe2(cd,O_DIRECT);
	if(err < 0) {
		fprintf(stderr,"child pipe\n");
		perror("ForkerThread");
		pthread_exit(NULL);
	}

	pid = fork();
	if(pid == 0) {
		/*
		 * child doesn't need read end of its pipe
		 */
		close(cd[0]);
		/*
		 * child reads from stdin
		 */
		close(0);
		dup2(pd[0],0);
		/*
		 * child uses stderr to write back to parent
		 */
		close(2);
		dup2(cd[1],2);
		fargv[0]="fork-proc";
		fargv[1]=NULL;
		execve("fork-proc",fargv,NULL);
		fprintf(stderr,"execve failed\n");
		fflush(stderr);
		pthread_exit(NULL);
	} else if(pid < 0) {
		fprintf(stderr,"fork failed\n");
		fflush(stderr);
		pthread_exit(NULL);
	}
	
	/*
	 * parent code here
	 */

	/*
	 * parnet doesn't need read end of pd or write end of cd
	 */
	close(pd[0]);
	close(cd[1]);

	while(1) {
		P(&Throttle);
#ifdef DEBUG
		printf("thread %d running\n",ta->tid);
#endif
		pthread_mutex_lock(&CLock);

		/*
		 * finihsed counts completions
		 */
		if(Finished == Total) {
			pthread_mutex_unlock(&CLock);
			close(pd[1]); /* should cause child tp exit */
			V(&Throttle);
#ifdef DEBUG
			printf("thread %d exiting\n",ta->tid);
#endif
			pthread_exit(NULL);
		}

		if(Counter == 0) {
#ifdef DEBUG
			printf("thread %d Counter: %d Finished: %d Total: %d\n",
				ta->tid,Counter,Finished,Total);
#endif
			pthread_mutex_unlock(&CLock);
//			V(&Throttle);
			continue;
		}
		my_count = Counter;
		Counter--;
		pthread_mutex_unlock(&CLock);

		memset(p2,0,sizeof(p1));
		sprintf(p2,"%d",my_count);
		memset(p1,0,sizeof(p1));
		strcpy(p1,ta->handler);

#ifdef DEBUG
		printf("thread %d sending %s\n",ta->tid,p2);
#endif

		err = write(pd[1],p1,sizeof(p1));
		if(err < 0) {
			fprintf(stderr,"first write failed\n");
			perror("ForkerThread");
			pthread_exit(NULL);
		}
		err = write(pd[1],p2,sizeof(p2));
		if(err < 0) {
			fprintf(stderr,"second write failed\n");
			perror("ForkerThread");
			pthread_exit(NULL);
		}

		/*
		 * wait for child to indicate done
		 */
#ifdef DEBUG
		printf("thread %d about to wait\n",ta->tid);
#endif
		err = read(cd[0],&c,1);
#ifdef DEBUG
		printf("thread %d recvd %d back\n",ta->tid,err);
#endif
		if(err > 0) {
			pthread_mutex_lock(&CLock);
			Finished++;
#ifdef DEBUG
			printf("thread %d incrementing finish to %d\n",
				ta->tid,Finished);
#endif
			pthread_mutex_unlock(&CLock);
			V(&Throttle);
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
		tas[i].tid = i;
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
	
	

