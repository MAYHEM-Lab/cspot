#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "put-test.h"

#define ARGS "c:f:s:N:H:Lt:S"
char *Usage = "put-test -f woof_name for experiment (matching recv side)\n\
\t-H namelog-path\n\
\t-s size (payload size)\n\
\t-c count (number of payloads to send)\n\
\t-L use same namespace for source and target\n\
\t-N target namespace (as a URI)\n\
\t-S <run simple test with no handler>\n\
\t-t threads\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];
int UseLocal;
int Threads;
int Simple;

#define MAX_RETRIES 20

pthread_mutex_t Lock;
int Curr;
int Size;
int Count;
unsigned long Max_seq_no;

void *PutThread(void *arg)
{
	TARGS *ta = (TARGS *)arg;
	PT_EL *el = ta->el;
	unsigned long e_seq_no;
	PL *payload;

	payload = (PL *)malloc(Size);
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

		gettimeofday(&payload->tm,NULL);
		if(Simple == 0) {
			e_seq_no = WooFPut(ta->target_name,"recv",payload);
		} else {
			e_seq_no = WooFPut(ta->target_name,NULL,payload);
		}
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
	PL s_pl;
	PL e_pl;
	TARGS ta;
	char remote_log[1024];
	char remote_target[1024];
	

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
			case 'S':
				Simple = 1;
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
//	sprintf(el.target_name,"%s/%s.%s",NameSpace,Fname,"target");
	sprintf(el.target_name,"%s.%s",Fname,"target");
	memset(remote_target,0,sizeof(remote_target));
	sprintf(remote_target,"%s/%s.%s",NameSpace,Fname,"target");
	memset(el.log_name,0,sizeof(el.log_name));
//	sprintf(el.log_name,"%s/%s.%s",NameSpace,Fname,"log");
	memset(remote_log,0,sizeof(remote_log));
	sprintf(remote_log,"%s/%s.%s",NameSpace,Fname,"log");
	sprintf(el.log_name,"%s.%s",Fname,"log");
	el.element_size = Size;
	el.history_size = Count;
	

	/*
	 * start the experiment -- creates woofs on recv side
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
		err = WooFGet(remote_log,&elog,1);
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
		for(i=0; i < Count; i++) {
			gettimeofday(&pl->tm,NULL);
			if(Simple == 1) {
				e_seq_no = WooFPut(remote_target,NULL,pl);
			} else {
				e_seq_no = WooFPut(remote_target,"recv",pl);
			}
		}
		if(e_seq_no > Max_seq_no) {
			Max_seq_no = e_seq_no;
		}
		free(payload_buf);
	} else {
		pthread_mutex_init(&Lock,NULL);
		Curr = Count;
		tids = (pthread_t *)malloc(Threads*sizeof(pthread_t));
		if(tids == NULL) {
			exit(1);
		}
		ta.el = &el;
		ta.target_name = remote_target;
		for(i=0; i < Threads; i++) {
			err = pthread_create(&tids[i],NULL,PutThread,(void *)&ta);
			if(err < 0) {
				exit(1);
			}
		}
		for(i=0; i < Threads; i++) {
			pthread_join(tids[i],&status);
		}
		e_seq_no = Max_seq_no;
	}

	
	payload_buf = malloc(Size);
	if(payload_buf == NULL) {
		exit(1);
	}
	err = WooFGet(remote_target,payload_buf,1);
	if(err < 0) {
		fprintf(stderr,"couldn't get start ts\n");
		exit(1);
	}
	memcpy(&s_pl,payload_buf,sizeof(PL));
	err = WooFGet(remote_target,payload_buf,Max_seq_no);
	if(err < 0) {
		fprintf(stderr,"couldn't get end ts\n");
		exit(1);
	}
	memcpy(&e_pl,payload_buf,sizeof(PL));
	free(payload_buf);

	if(Simple == 0){
		/*
		 * now poll looking for end of the log
		 */
		retries = 0;
		do {
			sleep(1);
			/*
			 * log record woof contains one more record than the target
			 */
			err = WooFGet(remote_log,&elog,e_seq_no + 1);
			if(err > 0) {
				break;
			}
			retries++;
		} while(retries < MAX_RETRIES);

		if(retries >= MAX_RETRIES) {
			fprintf(stderr,"put-test: failed to see end of experiment\n");
			fflush(stderr);
			exit(1);
		}

		memcpy(&stop_tm,&(elog.tm),sizeof(struct timeval));

		/*
		 * get the start time
		 */
		err = WooFGet(remote_log,&elog,2);
		if(err < 0) {
			fprintf(stderr,"put-test: failed to get start of experiment\n");
			fflush(stderr);
			exit(1);
		}
		memcpy(&start_tm,&(elog.tm),sizeof(struct timeval));
	}

	if(Simple == 0) {
		elapsed = (double)(stop_tm.tv_sec * 1000000 + stop_tm.tv_usec) -
			(double)(start_tm.tv_sec * 1000000 + start_tm.tv_usec); 
		elapsed = elapsed / 1000000.0;

		bw = (double)(Size * (Count-1)) / elapsed;
		bw = bw / 1000000.0;
		elapsed = (double)(e_pl.tm.tv_sec * 1000000 + e_pl.tm.tv_usec) -
			(double)(s_pl.tm.tv_sec * 1000000 + s_pl.tm.tv_usec); 
		elapsed = elapsed / 1000000.0;

		printf("woof: %s elapsed: %fs, puts: %lu, bw: %f MB/s %f puts/s\n",
			arg_name,elapsed,Max_seq_no,bw,(double)Max_seq_no/elapsed);
	} else {
		elapsed = (double)(e_pl.tm.tv_sec * 1000000 + e_pl.tm.tv_usec) -
			(double)(s_pl.tm.tv_sec * 1000000 + s_pl.tm.tv_usec); 
		elapsed = elapsed / 1000000.0;
		bw = (double)(Size * Max_seq_no) / elapsed;
		bw = bw / 1000000.0;
		printf("woof: %s elapsed: %fs, puts: %lu, bw: %f MB/s %f puts/s\n",
			arg_name,elapsed,Max_seq_no,bw,(double)Max_seq_no/elapsed);
	}
	fflush(stdout);

	return(1);
}
