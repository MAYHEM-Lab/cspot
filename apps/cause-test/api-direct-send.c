#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "cause.h"

#define ARGS "W:F:"
char *Usage = "api-send -W remote_woof_name -F local_woof_name\n";

char Wname[4096];
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

	if (Fname[0] == 0)
	{
		fprintf(stderr, "must specify file name\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	if (Wname[0] == 0)
	{
		fprintf(stderr, "must specify woof name\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	memset(el.string, 0, sizeof(el.string));
	strncpy(el.string, "my first bark", sizeof(el.string));

	ndx = WooFPut(Wname, "cause_recv", (void *)&el);

	if (WooFInvalid(ndx))
	{
		fprintf(stderr, "WooFPut failed for %s\n", Wname);
		fflush(stderr);
		exit(1);
	}

	return (0);
}
