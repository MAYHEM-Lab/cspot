#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "repair.h"

#define ARGS "W:H:"
char *Usage = "repair -W woof_name\n";

char Wname[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	REPAIR_EL el;
	unsigned long ndx;
	char handler[256];
	memset(handler, 0, sizeof(handler));

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'W':
			strncpy(Wname, optarg, sizeof(Wname));
			break;
		case 'H':
			strncpy(handler, optarg, sizeof(handler));
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

	memset(el.string, 0, sizeof(el.string));
	strncpy(el.string, "my first bark", sizeof(el.string));

	if (handler[0] != 0)
	{
		ndx = WooFPut(Wname, handler, (void *)&el);
	}
	else
	{
		ndx = WooFPut(Wname, NULL, (void *)&el);
	}

	if (WooFInvalid(err))
	{
		fprintf(stderr, "first WooFPut failed for %s\n", Wname);
		fflush(stderr);
		exit(1);
	}

	return (0);
}
