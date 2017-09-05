#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"

#define ARGS "c:f:N:H:Lt:"
char *Usage = "invoke-test -f woof_name for experiment\n\
\t-H namelog-path\n\
\t-c count (number of invokes)\n\
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

pthread_mutex_t Lock;
int Curr;
int Size;
int Count;
unsigned long Max_seq_no;

void *PutThread(void *arg)
{
	unsigned long e_seq_no;
	struct timeval tm;

	while(1) {
		pthread_mutex_lock(&Lock);
		if(Curr <= 0) {
			pthread_mutex_unlock(&Lock);
			pthread_exit(NULL);
		}
		Curr--;

		pthread_mutex_unlock(&Lock);

		gettimeofday(&tm,NULL);
		e_seq_no = WooFPut(Fname,"invoke_handler",&tm);
		if(WooFInvalid(e_seq_no)) {
			pthread_exit(NULL);
		}

	}

	pthread_exit(NULL);
}


int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	pthread_t *tids;
	void *status;
	struct timeval tm;
	unsigned long e_seq_no;
	

	Size = 0;
	Count = 0;
	UseLocal = 0;

	Threads = 1;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
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

	err = WooFCreate(Fname,sizeof(struct timeval),Count);
	if(err < 0) {
		fprintf(stderr,"invoke-test: failed to create %s\n",Fname);
		fflush(stderr);
		exit(1);
	}
	if(Threads == 1) {
		for(i=0; i < Count; i++) {
			gettimeofday(&tm,NULL);
			e_seq_no = WooFPut(Fname,"invoke_handler",&tm);
		}
	} else {
		pthread_mutex_init(&Lock,NULL);
		Curr = Count;
		tids = (pthread_t *)malloc(Threads*sizeof(pthread_t));
		if(tids == NULL) {
			exit(1);
		}
		for(i=0; i < Threads; i++) {
			err = pthread_create(&tids[i],NULL,PutThread,NULL);
			if(err < 0) {
				exit(1);
			}
		}
		for(i=0; i < Threads; i++) {
			pthread_join(tids[i],&status);
		}
	}

	return(1);
}
