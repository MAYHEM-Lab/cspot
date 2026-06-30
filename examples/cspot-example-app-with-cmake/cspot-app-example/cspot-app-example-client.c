#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "woofc.h"
#include "cspot-app-example.h"

#define ARGS "W:S:"
char *Usage = "cspot-app-example-client -W woof_name\n\
\t-S number of round trips\n";

char Wname[4096];
char Iname[4096];
char Oname[4096];

#define MAX_RETRIES (1000)

/*
 * prints different in ms
 */
double Duration(struct timeval end, struct timeval start)
{
	double diff =
		((double)end.tv_sec + ((double)end.tv_usec / 1000000.0)) -
		((double)start.tv_sec + ((double)start.tv_usec / 1000000.0));

	return(diff * 1000.0);
}

int main(int argc, char **argv)
{
	char Iname[4096];
	char Oname[4096];
	EX_EL input_el;
	EX_EL output_el;
	unsigned long seq_no;
	unsigned long tail_seqno;
	int c;
	int i;
	int count = 0;
	struct timeval end;
	double post_duration;
	double rtt;
	int retry;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 'S':
				count = atoi(optarg);
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wname[0] == 0) {
		fprintf(stderr,"must specify woof name\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(count <= 0) {
		fprintf(stderr,"must specify round trip count\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}


	MAKE_EXTENDED_NAME(Iname,Wname,"input");
	MAKE_EXTENDED_NAME(Oname,Wname,"output");
	// if the name is not a valid URI, assume it is local
	// and intialize the fast path for local access
	if(!WooFValidURI(Iname)) {
		WooFInit();
	}

	memset(&input_el,0,sizeof(EX_EL));
	strncpy(&(input_el.woof_name[0]),Wname,sizeof(input_el.woof_name));

	// main loop
	for(i=0; i < count; i++) {
		// record the time when the put is posted
		gettimeofday(&input_el.posted,NULL);
		// put it to the input woof and specify the handler to fire
		seq_no = WooFPut(Iname,"cspot_app_example_handler",&input_el);
		if(WooFInvalid(seq_no)) {
			fprintf(stderr,"put failed in iteration %d\n",i);
			fflush(stderr);
			exit(1);
		}
		retry=0;
		// poll for the result until #retries looking for the input seq_no
		// to appear at the tail of the output seq_no
		tail_seqno = WooFGetLatestSeqno(Oname);
		if(WooFInvalid(tail_seqno)) {
			fprintf(stderr,"latest seqno invalid iteration %d\n",i);
			fflush(stderr);
			exit(1);
		}
		while((WooFGet(Oname,&output_el,0) < 0) ||
			       (output_el.i_seqno < seq_no))	{
			retry++;
			if(retry > MAX_RETRIES) {
				fprintf(stderr,"could not get output on iteration %d\n",i);
				fflush(stderr);
				exit(1);
			}
		}
		gettimeofday(&end,NULL);
		post_duration = Duration(output_el.fielded,input_el.posted);
		rtt = Duration(end,input_el.posted);
		printf("iteration %d: handler dispatch: %f ms, rtt: %f ms\n",
				i,
				post_duration,
				rtt);
	}
	exit(0);

}



