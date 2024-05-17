#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "woofc.h"
#include "stress-test.h"
#include "dlist.h"

#define ARGS "c:W:s:g:p:P:LlV"
char *Usage = "stress-test -W woof_name for stress test\n\
\t-L <use local woofs>\n\
\t-l <do latency test, otherwise throughput test>\n\
\t-s number of puts\n\
\t-g get threads\n\
\t-p put threads\n\
\t-P payload size\n\
\t-V <verbose>\n";

char Wname[4096];
char Iname[4096];
char Oname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

int PutRemaining;
pthread_mutex_t Plock;
pthread_mutex_t Glock;
pthread_cond_t Wait;

Dlist *Pending;
int Done;
int Payload_size;
double Total;
double Count;
int IsLatency;
int Verbose;


void *PutThread(void *arg)
{
	char Iname[4096];
	ST_EL *st;
	unsigned long seq_no;
	char *payload;
	int i;
	int size;

	MAKE_EXTENDED_NAME(Iname,Wname,"input");
	/*
	 * register backend is not thread safe?
	 * do this to prime the pump
	 *
	 * much faster when each thread does this
	 */
	pthread_mutex_lock(&Plock);
	seq_no = WooFGetLatestSeqno(Iname);
	pthread_mutex_unlock(&Plock);

	payload = (char *)malloc(Payload_size);
	if(payload == NULL) {
		exit(1);
	}
	memset(payload,0,Payload_size);
	st = (ST_EL *)payload;
//	memset(st,0,sizeof(ST_EL));
	strncpy(st->woof_name,Wname,sizeof(st->woof_name));
//printf("Put: %ld with %s %s %d\n",
//pthread_self(),
//Iname,
//Wname,
//Payload_size);
//fflush(stdout);
	pthread_mutex_lock(&Plock);
	while(PutRemaining > 0) {
		PutRemaining--;
		gettimeofday(&st->posted,NULL);
//printf("Put: pr: %d\n",PutRemaining);
		pthread_mutex_unlock(&Plock);
		seq_no = WooFPut(Iname,"stress_handler",st);
//printf("Put: seq_no: %ld\n",seq_no);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,"put thread failed\n");
			fflush(stderr);
			pthread_exit(NULL);
		}
		if(IsLatency == 1) {
			sleep(1);
		}

		pthread_mutex_lock(&Glock);
		DlistAppend(Pending,(Hval)seq_no);
		pthread_mutex_unlock(&Glock);

		pthread_mutex_lock(&Plock);
	}

	pthread_mutex_unlock(&Plock);

	free(payload);
	pthread_exit(NULL);
}

void *GetThread(void *arg)
{
	ST_EL st;
	char Oname[4096];
	unsigned long seq_no = -1;
	int err;
	double elapsed;
	DlistNode *dn;
	int retries;
	struct timespec ts;
	int started = 0;
	
	

	if(IsLatency == 0) {
		ts.tv_sec = 0;
		ts.tv_nsec = 1000000;  /* 2 ms wait time on get */
	}
	MAKE_EXTENDED_NAME(Oname,Wname,"output");
//printf("Get: %ld starting with %s\n",pthread_self(), Oname);
//fflush(stdout);
	while((Done == 0) || (Pending->first != NULL) ||
			(started == 0)) {
		if(IsLatency == 0) {
			nanosleep(&ts,NULL);
		}
		pthread_mutex_lock(&Glock);
		dn = Pending->first;
		if(dn != NULL) {
			DlistDelete(Pending,dn);
			pthread_mutex_unlock(&Glock);
			started = 1;
			retries = 0;
			seq_no = dn->value.l;
			while(retries < 30) {
				err = WooFGet(Oname,&st,seq_no);
				if(err < 0) {
					printf("get of seq_no %lu failed, retrying\n",seq_no);
					retries++;
					if(IsLatency == 1) {
						sleep(1);
					}
					continue;
				}
				break;
			}
			if(retries == 30) {
				printf("FAIL to get seq_no %lu\n",seq_no);
				fflush(stdout);
			} else {
				elapsed=(st.fielded.tv_sec * 1000000.0 + st.fielded.tv_usec) -
					(st.posted.tv_sec * 1000000.0 + st.posted.tv_usec);
				elapsed = elapsed / 1000;
				if(Verbose == 1) {
					printf("seq_no %lu latency %f\n",seq_no,elapsed);
				}
				pthread_mutex_lock(&Glock);
				Total += elapsed;
				Count++;
				pthread_mutex_unlock(&Glock);
			}
		} else {
			pthread_mutex_unlock(&Glock);
			if(IsLatency == 1) {
				sleep(1);
			}
		}
				
	}

	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int c;
	int err;
	char local_ns[1024];
	int size = 0;
	pthread_t *gtids;
	pthread_t *ptids;
	int gt;
	int pt;
	int i;
	int local;

	gt = 1;
	pt = 1;
	local = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 'g':
				gt = atoi(optarg);
				break;
			case 'p':
				pt = atoi(optarg);
				break;
			case 'P':
				Payload_size = atoi(optarg);
				break;
			case 'L':
				local = 1;
				break;
			case 'l':
				IsLatency = 1;
				break;
			case 'V':
				Verbose = 1;
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wname[0] == 0) {
		fprintf(stderr,"must specify woof name for experiment\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(size == 0){
		fprintf(stderr,"need to specify woof size\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(Payload_size < sizeof(ST_EL)) {
		Payload_size = sizeof(ST_EL);
	} 

	Pending = DlistInit();
	if(Pending == NULL) {
		exit(1);
	}

	gtids = (pthread_t *)malloc(gt * sizeof(pthread_t));
	if(gtids == NULL) {
		exit(1);
	}
	ptids = (pthread_t *)malloc(pt * sizeof(pthread_t));
	if(ptids == NULL) {
		exit(1);
	}

	pthread_mutex_init(&Plock,NULL);
	pthread_mutex_init(&Glock,NULL);
	PutRemaining = size;

//	char Iname[4096];
//	unsigned long seq_no;
//	MAKE_EXTENDED_NAME(Iname,Wname,"input");
//	pthread_mutex_lock(&Plock);
//	seq_no = WooFGetLatestSeqno(Iname);
//	pthread_mutex_unlock(&Plock);

	if(local == 1) {
		WooFInit();
	}

	for(i=0; i < pt; i++) {
		err = pthread_create(&ptids[i],NULL,PutThread,NULL);
		if(err < 0) {
			exit(1);
		}
	}

	/*
	 * if this is a latency test, start the get threads
	 */
	if(IsLatency == 1) {
		for(i=0; i < gt; i++) {
			err = pthread_create(&gtids[i],NULL,GetThread,NULL);
			if(err < 0) {
				exit(1);
			}
		}
	}

	/*
	 * otherwise, this is a throughput test.  Join with the put
	 * threads and then parse the Dllist with get threads
	 */

	for(i=0; i < pt; i++) {
		pthread_join(ptids[i],NULL);
	}
//printf("Joined with put threads\n");

	if(IsLatency == 0) {
		sleep(1);
		for(i=0; i < gt; i++) {
			err = pthread_create(&gtids[i],NULL,GetThread,NULL);
			if(err < 0) {
				exit(1);
			}
		}
	}

	Done = 1;
	for(i=0; i < gt; i++) {
		pthread_join(gtids[i],NULL);
	}

	free(gtids);
	free(ptids);
	DlistRemove(Pending);
	if(IsLatency == 1) {
		printf("avg latency: %f ms\n",Total/Count);
	} else {
		if(Total > 0) {
			printf("avg throughput: %f puts/s\n",Count / (Total/1000.0));
		} else {
			printf("No timing taken\n");
		}
	}
	
	return(1);
}


