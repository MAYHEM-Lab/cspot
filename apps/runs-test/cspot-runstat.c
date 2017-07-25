#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "woofc.h"
#include "cspot-runstat.h"
#include "c-twist.h"


#define ARGS "c:S:s:L:a:"
char *Usage = "usage: cspot-runstat -L logfile\n\
\t-a alpha level\n\
\t-c count\n\
\t-s sample_size\n\
\t-S seed\n";

uint32_t Seed;

char LogFile[2048];

int main(int argc, char **argv)
{
	int c;
	int count;
	int sample_size;
	int has_seed;
	struct timeval tm;
	double alpha;
	int err;
	FA fa;

	count = 100;
	sample_size = 100;
	has_seed = 0;
	alpha = 0.05;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'a':
				alpha = atof(optarg);
				break;
			case 'L':
				strncpy(LogFile,optarg,sizeof(LogFile));
				break;
			case 'c':
				count = atoi(optarg);
				break;
			case 's':
				sample_size = atoi(optarg);
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

	if(count <= 0) {
		fprintf(stderr,"count must be non-negative\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(sample_size <= 0) {
		fprintf(stderr,"sample_size must be non-negative\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if((alpha <= 0.0) || (alpha >= 1.0)) {
		fprintf(stderr,"alpha must be between 0 and 1\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}
		
	if(has_seed == 0) {
		gettimeofday(&tm,NULL);
		Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	}

	WooFInit(1);

	err = WooFCreate("Rargs",sizeof(FA),sample_size*100);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Rargs failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Sargs",sizeof(FA),count*100);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Sargs failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Kargs",sizeof(FA),count*100);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Kargs failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Rvals",sizeof(double),sample_size*3);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Rvals failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Svals",sizeof(double),count*3);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Rvals failed\n");
		fflush(stderr);
		exit(1);
	}

	CTwistInitialize(Seed);

	fa.i = 0;
	fa.j = 0;
	fa.count = count;
	fa.sample_size = count;
	fa.alpha = alpha;
	strncpy(fa.r,"Rvals",sizeof(fa.r));
	strncpy(fa.stats,"Svals",sizeof(fa.stats));

	if(LogFile[0] != 0) {
		strncpy(fa.logfile,LogFile,sizeof(fa.logfile));
	} else {
		fa.logfile[0] = 0;
	}

	err = WooFPut("Rargs","RHandler",&fa);

	if(err < 0) {
		fprintf(stderr,"main couldn't start");
		exit(1);
	}

	pthread_exit(NULL);

	return(0);
}

