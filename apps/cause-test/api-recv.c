#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "cause.h"

#define ARGS "f:N:H:W:"
char *Usage = "api-recv -f woof_name\n\
\t-H namelog-path to host wide namelog\n\
\t-N namespace\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
int UseNameSpace;

char putbuf1[1024];
char putbuf2[1024];

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
		case 'f':
		case 'W':
			strncpy(Fname, optarg, sizeof(Fname));
			break;
		case 'N':
			UseNameSpace = 1;
			strncpy(NameSpace, optarg, sizeof(NameSpace));
			break;
		case 'H':
			strncpy(Namelog_dir, optarg, sizeof(Namelog_dir));
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

	if (Namelog_dir[0] != 0)
	{
		sprintf(putbuf2, "WOOF_NAMELOG_DIR=%s", Namelog_dir);
		putenv(putbuf2);
	}

	if (UseNameSpace == 1)
	{
		sprintf(Wname, "woof://%s/%s", NameSpace, Fname);
		sprintf(putbuf1, "WOOFC_DIR=%s", NameSpace);
		putenv(putbuf1);
	}
	else
	{
		strncpy(Wname, Fname, sizeof(Wname));
	}

	WooFInit();

	err = WooFCreate(Wname, sizeof(CAUSE_EL), 5);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof from %s\n", Wname);
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return (0);
}
