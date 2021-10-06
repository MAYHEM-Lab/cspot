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

#define ARGS "c:S:s:L:a:r"
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
	double alpha;
	int err;
	FA fa;
	struct timeval tm;
	unsigned long ndx;
	int repair;

	count = 100;
	sample_size = 100;
	has_seed = 0;
	alpha = 0.05;
	repair = 0;

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
		case 's':
			sample_size = atoi(optarg);
			break;
		case 'S':
			Seed = atoi(optarg);
			has_seed = 1;
			break;
		case 'r':
			repair = 1;
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

	WooFInit();

	if (repair == 0)
	{
		err = WooFCreate("Rargs", sizeof(FA), sample_size * count + 1);
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate for Rargs failed\n");
			fflush(stderr);
			exit(1);
		}

		err = WooFCreate("Sargs", sizeof(FA), sample_size * count);
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate for Sargs failed\n");
			fflush(stderr);
			exit(1);
		}

		err = WooFCreate("Kargs", sizeof(FA), sample_size * count);
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate for Kargs failed\n");
			fflush(stderr);
			exit(1);
		}

		err = WooFCreate("Rvals", sizeof(double), sample_size * count);
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate for Rvals failed\n");
			fflush(stderr);
			exit(1);
		}

		err = WooFCreate("Svals", sizeof(double), count);
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate for Rvals failed\n");
			fflush(stderr);
			exit(1);
		}
	}

	CTwistInitialize(Seed);

	fa.i = 0;
	fa.j = 0;
	fa.count = count;
	fa.sample_size = sample_size;
	fa.alpha = alpha;

	/*
	 * load up woof names
	 *
	 * note that these will be interpreted relative to the same
	 * namespace which will be set by WOOFC_DIR when WooFInit() is
	 * called.
	 */
	strncpy(fa.rargs, "Rargs", sizeof(fa.rargs));
	strncpy(fa.r, "Rvals", sizeof(fa.r));

	strncpy(fa.sargs, "Sargs", sizeof(fa.sargs));
	strncpy(fa.stats, "Svals", sizeof(fa.stats));

	strncpy(fa.kargs, "Kargs", sizeof(fa.kargs));

	if (LogFile[0] != 0)
	{
		strncpy(fa.logfile, LogFile, sizeof(fa.logfile));
	}
	else
	{
		fa.logfile[0] = 0;
	}

	ndx = WooFPut("Rargs", "RHandler", &fa);

	if (WooFInvalid(ndx))
	{
		fprintf(stderr, "main couldn't start");
		exit(1);
	}

	pthread_exit(NULL);

	return (0);
}
