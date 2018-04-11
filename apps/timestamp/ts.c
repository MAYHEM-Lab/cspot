#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "woofc.h"
#include "ts.h"

long stamp() {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return spec.tv_sec * 1000 + spec.tv_nsec / 1e6;
}

int ts(WOOF *wf, unsigned long seq_no, void *ptr) {
	TS_EL *el = (TS_EL *)ptr;
	long timestamp = stamp();
	fprintf(stdout, "timestamp: %ld\n", timestamp);
	fprintf(stdout, "from woof %s at %lu\n", wf->shared->filename, seq_no);
	el->ts[el->head] = timestamp;
	WooFPut(wf->shared->filename, NULL, (void *)el);
	el->head++;
	if (el->head < 16 && el->woof[el->head][0] != 0) {
		// put el back to woof
		fprintf(stdout, "next WooF: %s\n", el->woof[el->head]);
		seq_no = WooFPut(el->woof[el->head], "ts", (void *)el);
		if (WooFInvalid(seq_no)) {
			fprintf(stderr, "couldn't put object to WooF %s\n", wf->shared->filename);
		} else {
			fprintf(stdout, "put object to WooF %s at %ld\n", wf->shared->filename, seq_no);
		}
	}
	fflush(stderr);
	fflush(stdout);
	return(1);
}
