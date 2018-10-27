#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "mastr-slave.h"

int MSPulseHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

	PULSE *pstc = (PULSE *)ptr;
	char state_woof[4096];
	STATE state_src;
	char l_state_woof[255];
	unsigned long seq_no;
	int err;

#ifdef DEBUG
	printf("MSPulseHandler: called on %s with seq_no: %lu, time: %d\n",
		pstc.wname,
		pstc.last_seq_no,
		(double)(pstc.tm.tv_sec));
#endif

	err = WooFNameFromURI(pstc.wname, l_state_woof, sizeof(l_state_woof));

	if(err < 0) {
		fprintf(stderr,"MSPulseHandler: couldn't extract local state woof name from %s\n",
			pstc.wname);
		fflush(stderr);
		exit(1);
	}

	return(1);

}
