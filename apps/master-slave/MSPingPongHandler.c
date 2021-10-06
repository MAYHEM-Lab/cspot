#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "master-slave.h"

#define DEBUG

int MSPingPongHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	PINGPONG *pp = (PINGPONG *)ptr;
	int err;
	PINGPONG ret_pp;

#ifdef DEBUG
	printf("PingPongHandler: called with seq_no %lu, remo seq_no %lu, ret woof: %s\n",
		seq_no,
		pp->seq_no,
		pp->return_woof);
	fflush(stdout);
#endif

	/*
	 * send the seqience number back to the originator
	 */
	ret_pp.seq_no = pp->seq_no;

	err = WooFPut(pp->return_woof,NULL,&ret_pp);
	if(err < 0) {
		fprintf(stderr,"PingPongHandler: failed to set %lu back to %s\n",
				ret_pp.seq_no,
				pp->return_woof);
		fflush(stderr);
		exit(1);
	}
#ifdef DEBUG
	printf("PingPongHandler: success sending seq_no %lu, to ret woof: %s\n",
		ret_pp.seq_no,
		pp->return_woof);
	fflush(stdout);
#endif

	exit(0);
}

	
