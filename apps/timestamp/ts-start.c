#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "ts.h"

#define ARGS "N:W:CS"
char *Usage = "ts-start -W woof_name -N count\n\
-C to create woof\n";

char Wname[4096];
char putbuf1[1024];
char putbuf2[1024];

int main(int argc, char **argv)
{
	int c;
	int err;
	TS_EL el;
	unsigned long seq_no;
	int count;
	int create;
	int again;

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
		case 'C':
			create = 1;
			break;
		case 'S':
			again = 1;
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

	if (create)
	{
		err = WooFCreate(Wname, sizeof(TS_EL), count);
		if (err < 0)
		{
			fprintf(stderr, "couldn't create woof from %s\n", Wname);
			fflush(stderr);
			exit(1);
		}
	}

	el.head = 0;
	int i;
	for (i = 0; i < 16; i++)
	{
		memset(el.woof[i], 0, 256);
	}
	el.stop = 2;
	if (create)
	{
		el.repair = 0;
	}
	else
	{
		el.repair = 1;
	}
	el.again = again;

	strcpy(el.woof[0], "woof://128.111.45.215:58721/home/fasthall/cspot/apps/timestamp/cspot/test");
	strcpy(el.woof[1], "woof://169.231.235.113:57934/home/centos/cspot/apps/timestamp/cspot/test");

	for (i = 0; i < count; i++)
	{
		seq_no = WooFPut(Wname, "ts", (void *)&el);
		if (WooFInvalid(seq_no))
		{
			fprintf(stderr, "WooFPut failed for %s\n", Wname);
			fflush(stderr);
			exit(1);
		}
	}

	pthread_exit(NULL);
	return (0);
}
