#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "ts.h"

#define ARGS "N:W:"
char *Usage = "ts-create -W woof_name -N count\n";

char Wname[4096];
char putbuf1[1024];
char putbuf2[1024];

int main(int argc, char **argv)
{
	int c;
	int err;
	TS_EL el;
	unsigned long ndx;
	int count;

	count = 1;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'N':
			count = atoi(optarg);
			break;
		case 'W':
			strncpy(Wname, optarg, sizeof(Wname));
			break;
		default:
			fprintf(stderr,
					"unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (Wname[0] == 0)
	{
		fprintf(stderr, "must specify woof name\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	err = WooFCreate(Wname, sizeof(TS_EL), count);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof from %s\n", Wname);
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return (0);
}
