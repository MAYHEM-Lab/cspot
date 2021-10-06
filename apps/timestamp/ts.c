#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "woofc.h"
#include "ts.h"

uint64_t stamp()
{
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	return (uint64_t)spec.tv_sec * 1000 + (uint64_t)spec.tv_nsec / 1e6;
}

int ts(WOOF *wf, unsigned long seq_no, void *ptr)
{
	uint64_t ts = stamp();

	TS_EL *el = (TS_EL *)ptr;
	fprintf(stdout, "from woof %s at %lu\n", WoofGetFileName(wf), seq_no);
	el->ts[el->head] = ts;
	el->head++;
	if (el->head == el->stop)
	{
		fprintf(stdout, "Reach the last stop\n");
		int i;
		for (i = 0; i < el->stop; i++)
		{
			fprintf(stdout, "timestamp of %s: %lld\n", el->woof[i], el->ts[i]);
		}
		if (el->again)
		{
			fprintf(stdout, "again_timestamp_diff: %lld\n", el->ts[el->stop - 1] - el->ts[0]);
		}
		else if (el->repair)
		{
			fprintf(stdout, "repair_timestamp_diff: %lld\n", el->ts[el->stop - 1] - el->ts[0]);
		}
		else
		{
			fprintf(stdout, "first_timestamp_diff: %lld\n", el->ts[el->stop - 1] - el->ts[0]);
		}
	}
	else
	{
		// put el back to woof
		fprintf(stdout, "next WooF: %s\n", el->woof[el->head]);
		seq_no = WooFPut(el->woof[el->head], "ts", (void *)el);
		if (WooFInvalid(seq_no))
		{
			fprintf(stderr, "couldn't put object to WooF %s\n", WoofGetFileName(wf));
		}
		else
		{
			fprintf(stdout, "put object to WooF %s at %ld\n", WoofGetFileName(wf), seq_no);
		}
	}
	fflush(stderr);
	fflush(stdout);
	return (1);
}
