#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "put-test.h"

#define ARGS "c:f:s:N:H:Lt:"
char *Usage = "bw-test -f woof_name for experiment (matching recv side)\n\
\t-H namelog-path\n\
\t-s size (payload size)\n\
\t-c count (number of payloads to send)\n\
\t-L use same namespace for source and target\n\
\t-N target namespace (as a URI)\n\
\t-t threads\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];
int UseLocal;
int Threads;

#define MAX_RETRIES 20

pthread_mutex_t Lock;
int Curr;
int Size;
int Count;
unsigned long Max_seq_no;

void *PutThread(void *arg)
{
	PT_EL *el = (PT_EL *)arg;
	unsigned long e_seq_no;
	void *payload;

	payload = malloc(Size);
	if(payload == NULL) {
		exit(1);
	}

	while(1) {
		pthread_mutex_lock(&Lock);
		if(Curr <= 0) {
			pthread_mutex_unlock(&Lock);
			free(payload);
			pthread_exit(NULL);
		}
		Curr--;

		pthread_mutex_unlock(&Lock);

		e_seq_no = WooFPut(el->target_name,"null_handler",payload);
		if(WooFInvalid(e_seq_no)) {
			free(payload);
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&Lock);
		if(Max_seq_no < e_seq_no) {
			Max_seq_no = e_seq_no;
		}
		pthread_mutex_unlock(&Lock);

	}

	pthread_exit(NULL);
}


int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	PT_EL el;
	unsigned long seq_no;
	unsigned long e_seq_no;
	unsigned long l_seq_no;
	struct timeval start_tm;
	struct timeval stop_tm;
	EX_LOG elog;
	double elapsed;
	double bw;
	char arg_name[4096];
	void *payload_buf;
	PL *pl;
	int retries;
	pthread_t *tids;
	void *status;
	

	Size = 0;
	Count = 0;
	UseLocal = 0;

	Threads = 1;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 's':
				Size = atoi(optarg);
				break;
			case 'N':
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			case 'H':
				strncpy(Namelog_dir,optarg,sizeof(Namelog_dir));
				break;
			case 'c':
				Count = atoi(optarg);
				break;
			case 'L':
				UseLocal = 1;
				break;
			case 't':
				Threads = atoi(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if((NameSpace[0] == 0) || (WooFValidURI(NameSpace) == 0)) {
		fprintf(stderr,"must specify namespace for experimen as URI\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(Size == 0) {
		fprintf(stderr,"must specify payload size\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(Count == 0) {
		fprintf(stderr,"must specify number of payloads\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
		

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	if(UseLocal == 1) {
		WooFInit();
	}

	memset(arg_name,0,sizeof(arg_name));
	sprintf(arg_name,"%s/%s",NameSpace,Fname);

	if(Size < sizeof(PL)) {
		Size = sizeof(PL);
	}

	memset(el.target_name,0,sizeof(el.target_name));
	sprintf(el.target_name,"%s/%s.%s",NameSpace,Fname,"target");
	memset(el.log_name,0,sizeof(el.log_name));
	sprintf(el.log_name,"%s/%s.%s",NameSpace,Fname,"log");
	el.element_size = Size;
	el.history_size = Count;
	

	/*
	 * start the experiment
	 */
	seq_no = WooFPut(arg_name,"recv_start",&el);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"put-test: failed to start the experiment with %s\n",arg_name);
		fflush(stderr);
		exit(1);
	}


	/*
	 * wait for the recv side to create its timing log
	 */
	retries = 0;
	do {
		sleep(1);
		err = WooFGet(el.log_name,&elog,1);
		if(err > 0) {
			break;
		}
		retries++;
	} while(retries < MAX_RETRIES);

	if(retries == MAX_RETRIES) {
		fprintf(stderr,"put-test: timed out wating for log init on remote end\n");
		fflush(stderr);
		exit(1);
	}


	/*
	 * assume that recv-init has been called to create the remote WOOF for args
	 */
	if(Threads == 1) {
		payload_buf = malloc(Size);
		if(payload_buf == NULL) {
			exit(1);
		}

		pl = (PL *)payload_buf;
		pl->exp_seq_no = seq_no;
		gettimeofday(&start_tm,NULL);
		for(i=0; i < Count; i++) {
			e_seq_no = WooFPut(el.target_name,"null_handler",pl);
		}
		gettimeofday(&stop_tm,NULL);
		free(payload_buf);
	} else {
		pthread_mutex_init(&Lock,NULL);
		Curr = Count;
		tids = (pthread_t *)malloc(Threads*sizeof(pthread_t));
		if(tids == NULL) {
			exit(1);
		}
		gettimeofday(&start_tm,NULL);
		for(i=0; i < Threads; i++) {
			err = pthread_create(&tids[i],NULL,PutThread,(void *)&el);
			if(err < 0) {
				exit(1);
			}
		}
		for(i=0; i < Threads; i++) {
			pthread_join(tids[i],&status);
		}
		gettimeofday(&stop_tm,NULL);
		e_seq_no = Max_seq_no;
	}


	elapsed = (double)(stop_tm.tv_sec * 1000000 + stop_tm.tv_usec) -
			(double)(start_tm.tv_sec * 1000000 + start_tm.tv_usec); 
	elapsed = elapsed / 1000000.0;

	bw = (double)(Size * Count) / elapsed;
	bw = bw / 1000000.0;

	printf("woof: %s bw: %f MB/s\n",arg_name,bw);
	fflush(stdout);

	return(1);
}
