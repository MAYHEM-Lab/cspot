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
char *Usage = "usage: cspot-runstat-fast -L logfile\n\
\t-a alpha level\n\
\t-c count\n\
\t-s sample_size\n\
\t-S seed\n";

uint32_t Seed;

char LogFile[2048];

int main(int argc, char **argv)
{
	int c;
	int i;
	int j;
	int count;
	int sample_size;
	int has_seed;
	struct timeval tm;
	double alpha;
	int err;
	FA fa;
	WOOF *s_wf;
	WOOF *r_wf;
	double r;
	unsigned long ndx;

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

	WooFInit();

	err = WooFCreate("Sargs",sizeof(FA),sample_size*count);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Sargs failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Kargs",sizeof(FA),sample_size*count);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Kargs failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Rvals",sizeof(double),sample_size*count);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Rvals failed\n");
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate("Svals",sizeof(double),count+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Rvals failed\n");
		fflush(stderr);
		exit(1);
	}

	CTwistInitialize(Seed);

	fa.count = count;
	fa.sample_size = sample_size;
	fa.alpha = alpha;
	strncpy(fa.r,"Rvals",sizeof(fa.r));
	strncpy(fa.stats,"Svals",sizeof(fa.stats));

	if(LogFile[0] != 0) {
		strncpy(fa.logfile,LogFile,sizeof(fa.logfile));
	} else {
		fa.logfile[0] = 0;
	}

	s_wf = WooFOpen("Sargs");
	if(s_wf == NULL) {
		fprintf(stderr,"couldn't open Sargs\n");
		exit(1);
	}

	r_wf = WooFOpen("Rvals");
	if(r_wf == NULL) {
		fprintf(stderr,"couldn't open Rvals\n");
		exit(1);
	}

	gettimeofday(&tm,NULL);
	Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	CTwistInitialize(Seed);

	for(i=0; i < count; i++) {
		for(j=0; j < sample_size; j++) {
			r = CTwistRandom();
			ndx = WooFAppend(r_wf,NULL,&r);
			if(WooFInvalid(ndx)) {
				fprintf(stderr,"failed to append at i: %d j: %d\n",
						i,j);
				exit(1);
			}
		}
		fa.i = i;
		fa.j = sample_size-1;
		fa.ndx = ndx;
		ndx = WooFAppend(s_wf,"SHandler",&fa);
		if(WooFInvalid(ndx)) {
			fprintf(stderr,"failed to append to Sargs\n");
			exit(1);
		}
	}

	WooFFree(s_wf);
	WooFFree(r_wf);


	pthread_exit(NULL);

	return(0);
}

