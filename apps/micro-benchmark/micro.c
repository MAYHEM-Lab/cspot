#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "woofc.h"
#include "micro.h"

int count = 1000;

int micro(WOOF *wf, unsigned long seq_no, void *ptr)
{
	MICRO_EL *el = (MICRO_EL *)ptr;
	unsigned long seqno;
	int i;
	struct timeval t1, t2;
	double elapsedTime;

	gettimeofday(&t1, NULL);
	for (i = 0; i < count; i++)
	{
		seqno = WooFGetLatestSeqno("test2");
	}
	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	printf("Timing:WooFGetLatestSeqno: %f\n", elapsedTime);
	fflush(stdout);

	return (1);
}
