#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "cause.h"

#define ARGS "F:"
char *Usage = "create -F local_woof_name\n";

char Fname[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	CAUSE_EL el;
	unsigned long ndx;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'F':
			strncpy(Fname, optarg, sizeof(Fname));
			break;
		default:
			fprintf(stderr,
					"unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (Fname[0] == 0)
	{
		fprintf(stderr, "must specify file name\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	err = WooFCreate(Fname, sizeof(CAUSE_EL), 10);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof from %s\n", Fname);
		fflush(stderr);
		exit(1);
	}

	return (0);
}
