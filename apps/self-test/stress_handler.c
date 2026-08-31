#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "woofc.h"
#include "stress-test.h"

char Oname[4096];


/*
 * put on the target and not on the WOOF with the args
 */
int stress_handler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	ST_EL *st = (ST_EL *)ptr;
	unsigned long o_seq_no;

	gettimeofday(&st->fielded,NULL);
	MAKE_EXTENDED_NAME(Oname,st->woof_name,"output");
	st->seq_no = seq_no;
//printf("HANDLER: putting %lu\n",seq_no);
	o_seq_no = WooFPut(Oname,NULL,st);
	if(WooFInvalid(o_seq_no)) {
		fprintf(stderr,"couldn't write woof to %s\n",Oname);
		fflush(stderr);
	}
//printf("HANDLER: putting %lu to %lu\n",seq_no,o_seq_no);

	return(1);
}

