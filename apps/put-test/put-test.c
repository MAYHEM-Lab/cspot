#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "put-test.h"

#define ARGS "c:f:s:N:H:"
char *Usage = "put-test -f woof_name for experiment (matching recv side)\n\
\t-H namelog-path\n\
\t-s size (payload size)\n\
\t-c count (number of payloads to send)\n\
\t-N target namespace (as a URI)\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int i;
	int size;
	int count;
	int err;
	PT_EL el;
	unsigned long seq_no;
	unsigned long e_seq_no;
	unsigned long l_seq_no;
	struct timeval start_tm;
	struct timeval stop_tm;
	EX_LOG *elog;
	double elapsed;
	double bw;
	char arg_name[4096];
	void *payload_buf;
	PL *pl;
	int retries;
	

	size = 0;
	count = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 'N':
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			case 'H':
				strncpy(Namelog_dir,optarg,sizeof(Namelog_dir));
				break;
			case 'c':
				count = atoi(optarg);
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

	if(size == 0) {
		fprintf(stderr,"must specify payload size\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(count == 0) {
		fprintf(stderr,"must specify number of payloads\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
		

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	memset(arg_name,0,sizeof(arg_name));
	sprintf(arg_name,"%s/%s",NameSpace,Fname);

	if(size < sizeof(PL)) {
		size = sizeof(PL);
	}

	memset(el.target_name,0,sizeof(el.target_name));
	sprintf(el.target_name,"%s/%s.%s",NameSpace,Fname,"target");
	memset(el.log_name,0,sizeof(el.log_name));
	sprintf(el.log_name,"%s/%s.%s",NameSpace,Fname,"log");
	el.element_size = size;
	el.history_size = count;
	

	WooFInit();

	/*
	 * start the experiment
	 */
	seq_no = WooFPut(arg_name,"recv-start",&el);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"put-test: failed to start the experiment with %s\n",arg_name);
		fflush(stderr);
		exit(1);
	}


	payload_buf = malloc(size);
	if(payload_buf == NULL) {
		exit(1);
	}

	pl = (PL *)payload_buf;
	pl->exp_seq_no = seq_no;

	/*
	 * assume that recv-init has been called to create the remote WOOF for args
	 */
	for(i=0; i < count; i++) {
		e_seq_no = WooFPut(el.target_name,"recv",pl);
	}

	/*
	 * now poll looking for end of the log
	 */
	retries = 0;
	do {
		err = WooFGet(el.log_name,&elog,e_seq_no);
		if(err > 1) {
			break;
		}
		retries++;
		sleep(1);
	} while(retries < MAX_RETRIES);

	if(retries >= MAX_RETRIES) {
		fprintf(stderr,"put-test: failed to see end of experiment\n");
		fflush(stderr);
		exit(1);
	}

	memcpy(&stop_tm,&(elog->tm),sizeof(struct timeval));

	/*
	 * get the start time
	 */
	err = WooFGet(el.log_name,&elog,1);
	if(err < 0) {
		fprintf(stderr,"put-test: failed to get start of experiment\n");
		fflush(stderr);
		exit(1);
	}
	memcpy(&start_tm,&(elog->tm),sizeof(struct timeval));

	elapsed = (double)(stop_tm.tv_sec * 1000000 + stop_tm.tv_usec) -
			(double)(start_tm.tv_sec * 1000000 + start_tm.tv_usec); 
	elapsed = elapsed / 1000000.0;

	bw = (double)(size * count) / elapsed;
	bw = bw / 1000000.0;

	printf("woof: %s bw: %f MB/s\n",arg_name,bw);
	fflush(stdout);

	pthread_exit(NULL);

	return(1);
}
