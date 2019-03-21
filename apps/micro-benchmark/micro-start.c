#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "micro.h"
#include "time.h"

#define ARGS "C:N:H:W:R"
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
	MICRO_EL el;
	unsigned long seq_no;
	count = 1000;
	int i;
	int repair;
	struct timeval t1, t2;
	double elapsedTime;

	repair = 0;

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
		case 'R':
			repair = 1;
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

	if (repair == 0)
	{
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
	}

	memset(el.payload, 0, sizeof(el.payload));
	if (repair == 0)
	{
		seq_no = WooFPut(Wname2, NULL, (void *)&el);
		if (WooFInvalid(seq_no))
		{
			fprintf(stderr, "WooFPut failed for %s\n", Wname2);
			fflush(stderr);
			exit(1);
		}
	}

	// WooFPut
	gettimeofday(&t1, NULL);
	for (i = 0; i < 999; i++)
	{
		seq_no = WooFPut(Wname, NULL, (void *)&el);
	}
	if (WooFInvalid(seq_no))
	{
		fprintf(stderr, "WooFPut failed for %s\n", Wname);
		fflush(stderr);
		exit(1);
	}
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("Timing:WooFPut: %f\n", elapsedTime);
	fflush(stdout);

	// WooFGet
	gettimeofday(&t1, NULL);
	for (i = 0; i < count; i++)
	{
		seq_no = (rand() % 999) + 1;
		err = WooFGet(Wname, &el, seq_no);
	}
	if (err < 0)
	{
		fprintf(stderr, "WooFGet failed for %s\n", Wname);
		fflush(stderr);
		exit(1);
	}
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("Timing:WooFGet: %f\n", elapsedTime);
	fflush(stdout);

	// WooFGetLatestSeqno
	// if (repair)
	// {
		// seq_no = WooFPut(Wname, "micro", (void *)&el);
		// if (WooFInvalid(seq_no))
		// {
		// 	fprintf(stderr, "WooFPut failed for %s\n", Wname);
		// 	fflush(stderr);
		// 	exit(1);
		// }
	// }

	pthread_exit(NULL);
	return (0);
}
