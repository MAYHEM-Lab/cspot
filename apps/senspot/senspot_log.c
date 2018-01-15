#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"


/*
 * put on the target and not on the WOOF with the args
 */
int senspot_log(WOOF *wf, unsigned long seq_no, void *ptr)
{
	SENSPOT *spt = (SENSPOT *)ptr;

	fprintf(stdout,"seq_no: %lu %s recv type %c from %s and timestamp %s\n",
			seq_no,
			wf->shared->filename,
			spt->type,
			spt->ip_addr,
			ctime(&spt->tv_sec));
	fflush(stdout);
	return(1);
}

