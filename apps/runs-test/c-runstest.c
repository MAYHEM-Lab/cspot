#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "c-twist.h"

#define ARGS "c:TS:"
char *Usage = "usage: c-runstest -c count\n\
\t-S seed\n\
\t-T <use threads>\n";

int UseThread;
uint32_t Seed;

int main(int argc, char **argv)
{
	int c;
	int count;
	int i;
	int has_seed;
	struct timeval tm;
	double r;

	count = 10;
	UseThread = 0;
	has_seed = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'c':
				count = atoi(optarg);
				break;
			case 'T':
				UseThread = 1;
				break;
			case 'S':
				Seed = atoi(optarg);
				has_seed = 1;
				break;
			default:
				fprintf(stderr,"uncognized command -%c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(has_seed == 0) {
		gettimeofday(&tm,NULL);
		Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	}

	CTwistInitialize(Seed);

	for(i=0; i < count; i++) {
		r = CTwistRandom();
		printf("r: %f\n",r);
		fflush(stdout);
	}

	return(0);
}

	
				

