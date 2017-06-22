#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"

#define ARGS "f:s:"
char *Usage = "woofc-test1 -f filename\n\
\t-s size (in events)\n";

char Fname[4096];

int WH1(WOOF *wf, unsigned long long seq_no, void *element)
{
	unsigned long *counter;

	counter = (unsigned long *)element;

	printf("WH1: %llu counter: %lu\n",
			seq_no,
			*counter);
	fflush(stdout);

	return(1);
}

int main(int argc, char **argv)
{
	int c;
	int size;
	WOOF *wf_1;
	unsigned long counter;
	int err;

	size = 5;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
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

	wf_1 = WooFCreate(Fname,WH1,sizeof(unsigned long),size);

	if(wf_1 == NULL) {
		fprintf(stderr,"couldn't create wf_1\n");
		fflush(stderr);
		exit(1);
	}

	counter = 1;

	err = WooFPut(wf_1,&counter);

	if(err < 0) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);

	return(0);
}

