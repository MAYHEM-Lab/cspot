#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <time.h>

#include "woofc.h"
#include "cspot-runstat.h"
#include "c-twist.h"

/*
 * distributed version
 * woofs must be created before this is run
 */

#define ARGS "Cc:S:s:L:a:E:R:K:S:"
char *Usage = "usage: cspot-runstat-message -L logfile\n\
\t-a alpha level\n\
\t-c count\n\
\t-C <create the woofs>\n\
\t-s sample_size\n\
\t-K kwoofname\n\
\t-R rwoofname\n\
\t-S swoofname\n\
\t-E seed\n";

uint32_t Seed;

char LogFile[2048];
char Rwoof[2048];
char Kwoof[2048];
char Swoof[2048];
char Args[2048];

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
	int create;

	count = 100;
	sample_size = 100;
	has_seed = 0;
	alpha = 0.05;
	create = 0;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'a':
			alpha = atof(optarg);
			break;
		case 'L':
			strncpy(LogFile, optarg, sizeof(LogFile));
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'C':
			create = 1;
			break;
		case 's':
			sample_size = atoi(optarg);
			break;
		case 'E':
			Seed = atoi(optarg);
			has_seed = 1;
			break;
		case 'R':
			strncpy(Rwoof,optarg,sizeof(Rwoof));
			break;
		case 'S':
			strncpy(Swoof,optarg,sizeof(Swoof));
			break;
		case 'K':
			strncpy(Kwoof,optarg,sizeof(Kwoof));
			break;
		default:
			fprintf(stderr, "uncognized command -%c\n",
					(char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (count <= 0)
	{
		fprintf(stderr, "count must be non-negative\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	if (sample_size <= 0)
	{
		fprintf(stderr, "sample_size must be non-negative\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	if ((alpha <= 0.0) || (alpha >= 1.0))
	{
		fprintf(stderr, "alpha must be between 0 and 1\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	if (has_seed == 0)
	{
		gettimeofday(&tm, NULL);
		Seed = (uint32_t)((tm.tv_sec + tm.tv_usec) % 0xFFFFFFFF);
	}


	if (create == 1)
	{
		WooFInit();
		if(Rwoof[0] != 0) {
			memset(Args,0,sizeof(Args));
			strcpy(Args,Rwoof);
			strcat(Args,".args");
			err = WooFCreate(Args, sizeof(FA), sample_size * count + 1);
			if (err < 0)
			{
				fprintf(stderr, "WooFCreate for %s failed\n",
					Args);
				fflush(stderr);
				exit(1);
			}

			memset(Args,0,sizeof(Args));
			strcpy(Args,Rwoof);
			strcat(Args,".vals");
			err = WooFCreate(Args, sizeof(double), sample_size * count);
			if (err < 0)
			{
				fprintf(stderr, "WooFCreate for %s failed\n",Args);
				fflush(stderr);
				exit(1);
			}
		}

		if(Swoof[0] != 0) {
			memset(Args,0,sizeof(Args));
			strcpy(Args,Swoof);
			strcat(Args,".args");
			err = WooFCreate(Args, sizeof(FA), sample_size * count);
			if (err < 0)
			{
				fprintf(stderr, "WooFCreate for %s failed\n", Args);
				fflush(stderr);
				exit(1);
			}

			memset(Args,0,sizeof(Args));
			strcpy(Args,Swoof);
			strcat(Args,".vals");
			err = WooFCreate(Args, sizeof(double), count);
			if (err < 0)
			{
				fprintf(stderr, "WooFCreate for %s failed\n",Args);
				fflush(stderr);
				exit(1);
			}
		}

		if(Kwoof[0] != 0) {
			memset(Args,0,sizeof(Args));
			strcpy(Args,Kwoof);
			strcat(Args,".args");
			err = WooFCreate(Args, sizeof(FA), sample_size * count);
			if (err < 0)
			{
				fprintf(stderr, "WooFCreate for %s failed\n",
					Args);
				fflush(stderr);
				exit(1);
			}
		}

	} else { /* not creating */

		if((Rwoof[0] == 0) || (Swoof[0] == 0) || (Kwoof[0] == 0)) {
			fprintf(stderr,"must specify R, S,a nd K woofs to run\n");
			fprintf(stderr,"%s",Usage);
			exit(1);
		}
		

		CTwistInitialize(Seed);

		memset(&fa,0,sizeof(fa));

		strncpy(fa.rargs, Rwoof, sizeof(fa.rargs));
		strncat(fa.rargs, ".args", sizeof(fa.rargs));
		strncpy(fa.r, Rwoof, sizeof(fa.r));
		strncat(fa.r, ".vals", sizeof(fa.r));
		strncpy(fa.sargs, Swoof, sizeof(fa.sargs));
		strncat(fa.sargs, ".args", sizeof(fa.sargs));
		strncpy(fa.stats, Swoof, sizeof(fa.stats));
		strncat(fa.stats, ".vals", sizeof(fa.stats));
		strncpy(fa.kargs, Kwoof, sizeof(fa.kargs));
		strncat(fa.kargs, ".args", sizeof(fa.kargs));
		fa.i = 0;
		fa.j = 0;
		fa.count = count;
		fa.sample_size = sample_size;
		fa.alpha = alpha;

		ndx = WooFPut(fa.rargs, "RHandler", &fa);

		if (WooFInvalid(ndx))
		{
			fprintf(stderr, "main couldn't start");
			exit(1);
		}

	}
	pthread_exit(NULL);

	return (0);
}
