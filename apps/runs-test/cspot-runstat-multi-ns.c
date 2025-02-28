#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "woofc.h"
#include "cspot-runstat.h"
#include "c-twist.h"

/*
 * this version uses top_dir to create three name spaces
 *
 * the logic is that
 * RHandler gets its arguments in Rspace and puts random values to a WOOF in Sspace
 * Shandler gets its arguments in Sspace and reads values from WOOF in Sspace and, 
 *   after sample_have been accumulated, buts a Runs stat to a WOOF in Kspace
 * KHandler gets its arguments in Kspace and reads the Runs stats in Kspace and when count 
 *   stats have been accumulated generates a z-transform Normal distribution and runs a KS 
 *   test against the set of accumulated stats
 */

char Top_dir[2048];

#define ARGS "c:S:s:L:a:T:"
char *Usage = "usage: cspot-runstat-multi-ns -L logfile\n\
\t-T top_dir\n\
\t-a alpha level\n\
\t-c count\n\
\t-s sample_size\n\
\t-S seed\n";

uint32_t Seed;

char LogFile[2048];
char putbuf0[2048];
char putbuf1[2048];

int main(int argc, char **argv)
{
	int c;
	int count;
	int sample_size;
	int has_seed;
	double alpha;
	int err;
	FA fa;
	struct timeval tm;
	unsigned long ndx;
	pid_t pid;
	char start_args[1024];

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
			case 'T':
				strncpy(Top_dir,optarg,sizeof(Top_dir));
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

	if(Top_dir[0] == 0) {
		/*
		 * use CWD as the top dir for name spaces if it is not specified
		 *
	 	 * note that the name spaces must already be created under Top_dir 
	 	 * and loaded handlers and woofc-container
	 	 */
		getcwd(Top_dir,sizeof(Top_dir));
	}

		
	if(has_seed == 0) {
		gettimeofday(&tm,NULL);
		Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	}

	/*
	 * make the stats namespace
	 *
	 * note that WooFInit reads env variables and if it doesn't see
	 * one's explicitly naming directories, it will use CWD
	 */

	pid = fork();
	if(pid == 0) {
		sprintf(putbuf0,"WOOFC_DIR=%s/Sspace",Top_dir);
		putenv(putbuf0);
		WooFInit();
		/*
		 * create Sargs in Sspace for SHandler
		 */
		err = WooFCreate("Sargs",sizeof(FA),sample_size*count);
		if(err < 0) {
			fprintf(stderr,"WooFCreate for Sargs failed\n");
			fflush(stderr);
			exit(1);
		}

		/*
		 * create Rvals in Sspace so RHhandler has a landing spot for
		 * its values
		 */
		err = WooFCreate("Rvals",sizeof(double),sample_size*count);
		if(err < 0) {
			fprintf(stderr,"WooFCreate for Rvals failed\n");
			fflush(stderr);
			exit(1);
		}
		exit(0);
	} else {
		pid = wait(NULL);
	}

	pid = fork();
	if(pid == 0) {
		sprintf(putbuf0,"WOOFC_DIR=%s/Kspace",Top_dir);
		putenv(putbuf0);
		WooFInit();
		/*
		 * create Kargs in Kspace for KHandler
		 */
		err = WooFCreate("Kargs",sizeof(FA),sample_size*count);
		if(err < 0) {
			fprintf(stderr,"WooFCreate for Sargs failed\n");
			fflush(stderr);
			exit(1);
		}
		/*
		 * create Svals in Kspace so Shandler has a landing spot for the stats
		 */
		err = WooFCreate("Svals",sizeof(double),count);
		if(err < 0) {
			fprintf(stderr,"WooFCreate for Svals failed\n");
			fflush(stderr);
			exit(1);
		}
		exit(0);
	} else {
		pid = wait(NULL);
	}

	/*
	 * finally, create Rargs in Rspace
	 */
	sprintf(putbuf0,"WOOFC_DIR=%s/Rspace",Top_dir);
	putenv(putbuf0);
	WooFInit();

	err = WooFCreate("Rargs",sizeof(FA),sample_size*count);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for Rargs failed\n");
		fflush(stderr);
		exit(1);
	}


	CTwistInitialize(Seed);

	fa.i = 0;
	fa.j = 0;
	fa.count = count;
	fa.sample_size = sample_size;
	fa.alpha = alpha;
	/*
	 * load up the woof names
	 */
	sprintf(fa.rargs,"woof://%s/Rspace/Rargs",Top_dir);
	sprintf(fa.r,"woof://%s/Sspace/Rvals",Top_dir);

	sprintf(fa.sargs,"woof://%s/Sspace/Sargs",Top_dir);
	sprintf(fa.stats,"woof://%s/Kspace/Svals",Top_dir);

	sprintf(fa.kargs,"woof://%s/Kspace/Kargs",Top_dir);

	if(LogFile[0] != 0) {
		strncpy(fa.logfile,LogFile,sizeof(fa.logfile));
	} else {
		fa.logfile[0] = 0;
	}

	memset(start_args,0,sizeof(start_args));
	sprintf(start_args,"woof://%s/Rspace/Rargs",Top_dir);

	ndx = WooFPut(start_args,"RHandler",&fa);

	if(WooFInvalid(ndx)) {
		fprintf(stderr,"main couldn't start");
		exit(1);
	}

	pthread_exit(NULL);

	return(0);
}

