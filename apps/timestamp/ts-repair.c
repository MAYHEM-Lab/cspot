#define DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
#include "woofc-host.h"
#include "repair.h"

#define ARGS "N:D"

extern unsigned long Name_id;

int Repair(char *wf_name, int count);

int dryrun;

int main(int argc, char **argv)
{
	int i;
	int c;
	int err;
	int count;
	struct timeval t1, t2;
	double elapsedTime;

	dryrun = 0;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'N':
			count = atoi(optarg);
			break;
		case 'D':
			dryrun = 1;
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fflush(stderr);
			exit(1);
		}
	}

	WooFInit();
	err = Repair("test", count);
	if (err < 0)
	{
		fprintf(stderr, "couldn't repair RPi\n");
		fflush(stderr);
		exit(1);
	}

	err = Repair("woof://169.231.235.113:57934/home/centos/cspot/apps/timestamp/cspot/test", count);
	if (err < 0)
	{
		fprintf(stderr, "couldn't repair cloud\n");
		fflush(stderr);
		exit(1);
	}

	return (0);
}

int Repair(char *wf_name, int count)
{
	Dlist *holes;
	int err;
	Hval hval;
	int i;

	holes = DlistInit();
	if (holes == NULL)
	{
		fprintf(stderr, "Repair: cannot allocate Dlist\n");
		fflush(stderr);
		return (-1);
	}

	for (i = 1; i <= count; i++)
	{
		hval.i64 = i;
		DlistAppend(holes, hval);
	}

	printf("Repair: repairing %s: %d holes\n", wf_name, holes->count);
	fflush(stdout);

	if (holes->count > 0)
	{
		if (dryrun == 0)
		{
			err = WooFRepair(wf_name, holes);
			if (err < 0)
			{
				fprintf(stderr, "Repair: cannot repair woof %s\n", wf_name);
				fflush(stderr);
				DlistRemove(holes);
				return (-1);
			}
		}
	}
	DlistRemove(holes);
	return (0);
}
