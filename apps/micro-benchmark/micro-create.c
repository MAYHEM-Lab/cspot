#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "micro.h"
#include "time.h"

#define ARGS "C:N:H:W:"
char *Usage = "micro-start -f woof_name\n\
\t-H namelog-path to host wide namelog\n\
\t-N namespace\n";

char Wname[4096];
char Wname2[4096];

int main(int argc, char **argv)
{
	int c;
	int count;
	int err;
	count = 1000;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'C':
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
		fprintf(stderr, "must specify filename for woof\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	err = WooFCreate(Wname, sizeof(MICRO_EL), count + 1);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof from %s\n", Wname);
		fflush(stderr);
		exit(1);
	}
	sprintf(Wname2, "%s2", Wname);
	err = WooFCreate(Wname2, sizeof(MICRO_EL), count + 1);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof from %s\n", Wname2);
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return (0);
}
