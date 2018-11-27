#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "merge.h"

#define ARGS "W:"
char *Usage = "merge-remote -W woof_name\n";

char Fname[4096];
char Wname[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	MERGE_EL el;
	unsigned long ndx;
	int i;
	char buffer[255];

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'W':
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
		fprintf(stderr, "must specify filename for woof\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}
	strncpy(Wname, Fname, sizeof(Wname));

	WooFInit();

	err = WooFCreate(Wname, sizeof(MERGE_EL), 5);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof from %s\n", Wname);
		fflush(stderr);
		exit(1);
	}

	for (i = 0; i < 10; i++)
	{
		memset(el.string, 0, sizeof(el.string));
		sprintf(el.string, "my %dth bark", i + 1);
		ndx = WooFPut(Wname, "merge", (void *)&el);
		if (WooFInvalid(err))
		{
			fprintf(stderr, "%dth WooFPut failed for %s\n", i + 1, Wname);
			fflush(stderr);
			exit(1);
		}
	}

	return (0);
}
