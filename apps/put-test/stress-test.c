#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "woofc.h"
#include "put-test.h"
#include "dlist.h"

#define ARGS "c:W:s:N:H:g:p:P:"
char *Usage = "stress-test -W woof_name for stress test\n\
\t-H namelog-path\n\
\t-N target namespace (as a URI)\n\
\t-s number of puts\n\
\t-g get threads\n\
\t-p put threads\n\
\t-P payload size\n";

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

Dlist *Pending;
int Done;
int Payload_size;

void *PutThread(void *arg)
{
	ST_EL *st;
	char Iname[4096];
	unsigned long seq_no;
	char *payload;
	int i;

	MAKE_EXTENDED_NAME(Iname,Wname,"input");
	payload = (char *)malloc(Payload_size);
	if(payload == NULL) {
		exit(1);
	}
	for(i=0; i < Payload_size; i++) {
		payload[i] = 75;
	}
	st = (ST_EL *)payload;
	memset(st,0,sizeof(ST_EL));
	strcpy(st->woof_name,Wname);
	pthread_mutex_lock(&Plock);
	while(PutRemaining > 0) {
		PutRemaining--;
		pthread_mutex_unlock(&Plock);
		gettimeofday(&st->posted,NULL);
		seq_no = WooFPut(Iname,"stress_handler",st);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,"put thread failed\n");
			fflush(stderr);
			pthread_exit(NULL);
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
	unsigned long seq_no;
	int err;
	double elapsed;
	DlistNode *dn;
	int retries;

	MAKE_EXTENDED_NAME(Oname,Wname,"output");
	while((Done == 0) || (Pending->first != NULL)) {
		pthread_mutex_lock(&Glock);
		dn = Pending->first;
		if(dn != NULL) {
			DlistDelete(Pending,dn);
			pthread_mutex_unlock(&Glock);
			retries = 0;
			seq_no = dn->value.l;
			while(retries < 30) {
				err = WooFGet(Oname,&st,seq_no);
				if(err < 0) {
					printf("get of seq_no %lu failed, retrying\n",seq_no);
					sleep(1);
					retries++;
					continue;
				}
				break;
			}
			if(retries == 30) {
				printf("FAIL to get seq_no %lu\n",seq_no);
				fflush(stdout);
			} else {
				/*elapsed=(st.fielded.tv_sec * 1000000.0 + st.fielded.tv_usec) -
					(st.posted.tv_sec * 1000000.0 + st.posted.tv_usec);
				elapsed = elapsed / 1000;*/
				printf("seq_no %lu elapsed %f\n",seq_no,elapsed);
			}
		} else {
			pthread_mutex_unlock(&Glock);
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

	gt = 1;
	pt = 1;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 'N':
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			case 'H':
				strncpy(Namelog_dir,optarg,sizeof(Namelog_dir));
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

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
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

	for(i=0; i < pt; i++) {
		err = pthread_create(&ptids[i],NULL,PutThread,NULL);
		if(err < 0) {
			exit(1);
		}
	}

	for(i=0; i < gt; i++) {
		err = pthread_create(&gtids[i],NULL,GetThread,NULL);
		if(err < 0) {
			exit(1);
		}
	}

	for(i=0; i < pt; i++) {
		pthread_join(ptids[i],NULL);
	}
	Done = 1;
	for(i=0; i < gt; i++) {
		pthread_join(gtids[i],NULL);
	}

	free(gtids);
	free(ptids);
	DlistRemove(Pending);
	
	return(1);
}


