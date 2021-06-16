#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "micro.h"
#include "time.h"

#define ARGS "C:N:H:W:"
char *Usage = "micro-start -f woof_name\n\
\t-H namelog-path to host wide namelog\n\
\t-N namespace\n";

char Wname[4096];

int main(int argc, char **argv)
{
	int c;
	int count;
	int err;
	MICRO_EL el;
	unsigned long seq_no;
	count = 100;
	clock_t start;
	int diff;
	int i;

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

	srand(time(NULL));
	WooFInit();

	memset(el.payload, 0, sizeof (el.payload));

	// WooFPut
	start = clock();
	for (i = 0; i < count; i++)
	{
		seq_no = WooFPut(Wname, NULL, (void *)&el);
	}
	if (WooFInvalid(seq_no))
	{
		fprintf(stderr, "WooFPut failed for %s\n", Wname);
		fflush(stderr);
		exit(1);
	}
	diff = (clock() - start) * 1000 / CLOCKS_PER_SEC;
	printf("Called WooFPut %d times: %d ms\n", count, diff);
	fflush(stdout);

	// WooFGet
	start = clock();
	for (i = 0; i < count; i++)
	{
		seq_no = (rand() % count) + 1;
		err = WooFGet(Wname, &el, seq_no);
	}
	if (err < 0)
	{
		fprintf(stderr, "WooFGet failed for %s\n", Wname);
		fflush(stderr);
		exit(1);
	}
	diff = (clock() - start) * 1000 / CLOCKS_PER_SEC;
	printf("Called WooFGet %d times: %d ms\n", count, diff);
	fflush(stdout);

	// WooFGetLatestSeqno
	start = clock();
	for (i = 0; i < count; i++)
	{
		seq_no = WooFGetLatestSeqno(Wname);
	}
	diff = (clock() - start) * 1000 / CLOCKS_PER_SEC;
	printf("Called WooFGetLatestSeqno %d times: %d ms\n", count, diff);
	fflush(stdout);

	pthread_exit(NULL);
	return (0);
}
