#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc-thread.h"

#define ARGS "c:f:s:"
char *Usage = "woofc-test1 -f filename\n\
\t-c max_counter\n\
\t-s size (in events)\n";

char Fname[4096];
char Wname1[4096];
char Wname2[4096];

struct wh_state
{
	WOOF *target_wf;
	unsigned long counter;
	unsigned long max;
};

typedef struct wh_state WHSTATE;

int WH1(WOOF *my_wf, unsigned long long seq_no, void *element)
{
	WHSTATE *whs = (WHSTATE *)element;
	WHSTATE my_state;
	unsigned long counter;
	unsigned long ndx;
	int err;

	counter = whs->counter;

	if(counter == whs->max) {
		printf("WH1: %llu finsihed with counter: %lu max: %lu\n",
			seq_no,
			counter,
			whs->max);
		fflush(stdout);
		return(1);
	}

	printf("WH1: %llu counter: %lu, incrementing\n",
			seq_no,
			counter);

	counter = counter+1;
	my_state.counter = counter;
	my_state.max = whs->max;
	my_state.target_wf = my_wf;

	ndx = WooFPut(whs->target_wf,&my_state);

	fflush(stdout);

	return(1);
}

int WH2(WOOF *my_wf, unsigned long long seq_no, void *element)
{
	WHSTATE *whs = (WHSTATE *)element;
	WHSTATE my_state;
	unsigned long counter;
	int err;
	unsigned long ndx;

	counter = whs->counter;

	if(counter == whs->max) {
		printf("WH2: %llu finsihed with counter: %lu max: %lu\n",
			seq_no,
			counter,
			whs->max);
		fflush(stdout);
		return(1);
	}

	printf("WH2: %llu counter: %lu, incrementing\n",
			seq_no,
			counter);

	counter = counter+1;
	my_state.counter = counter;
	my_state.max = whs->max;
	my_state.target_wf = my_wf;

	ndx = WooFPut(whs->target_wf,&my_state);

	fflush(stdout);

	return(1);
}

int main(int argc, char **argv)
{
	int c;
	int size;
	WOOF *wf_1;
	WOOF *wf_2;
	unsigned long counter;
	unsigned long max_counter;
	int err;
	WHSTATE start;

	size = 5;
	max_counter = 10;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'c':
				max_counter = atoi(optarg);
				break;
			case 's':
				size = atoi(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for backing store\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	strcpy(Wname1,Fname);
	strcat(Wname1,".1");
	strcpy(Wname2,Fname);
	strcat(Wname2,".2");

	WooFInit();


	/*
	 * ping pong counter between two WooF objects
	 */
	wf_1 = WooFCreate(Wname1,WH1,sizeof(WHSTATE),size);

	if(wf_1 == NULL) {
		fprintf(stderr,"couldn't create wf_1\n");
		fflush(stderr);
		exit(1);
	}

	wf_2 = WooFCreate(Wname2,WH2,sizeof(WHSTATE),size);
	if(wf_2 == NULL) {
		fprintf(stderr,"couldn't create wf_2\n");
		fflush(stderr);
		exit(1);
	}

	start.counter = 1;
	start.max = max_counter;
	start.target_wf = wf_2;

	err = WooFPut(wf_1,&start);

	if(err < 0) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);

	return(0);
}

