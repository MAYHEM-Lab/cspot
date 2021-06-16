#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "woofc.h"
#include "hw.h"

uint64_t stamp() {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return (uint64_t)spec.tv_sec * 1000 + (uint64_t)spec.tv_nsec / 1e6;
}

int hw(WOOF *wf, unsigned long seq_no, void *ptr)
{
	HW_EL *el = (HW_EL *)ptr;
	el->server = stamp();
	fprintf(stdout,"from woof %s at %lu with string: %s\n",
			wf->shared->filename, seq_no, (char *)ptr);
	fprintf(stdout,"%llu %llu\n",el->client,el->server);
	fflush(stdout);
	return(1);
}
