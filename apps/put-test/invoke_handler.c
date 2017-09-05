#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "put-test.h"


/*
 * put on the target and not on the WOOF with the args
 */
int invoke_handler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	double elapsed;
	struct timeval tm;
	struct timeval *start = (struct timeval *)ptr;

	gettimeofday(&tm,NULL);

	elapsed = (double)(tm.tv_sec + (tm.tv_usec / 1000000.0)) -
		  (double)(start->tv_sec + (start->tv_usec / 1000000.0));
	elapsed = elapsed * 1000;

	printf("%f ms\n",elapsed);
	fflush(stdout);

	return(1);
}

