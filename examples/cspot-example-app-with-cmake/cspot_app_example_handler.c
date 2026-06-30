#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "woofc.h"
#include "cspot-app-example.h"

char Oname[4096];


/*
 * put on the target and not on the WOOF with the args
 */
int cspot_app_example_handler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	EX_EL *el = (EX_EL *)ptr;
	unsigned long o_seq_no;

	/*
	 * log the time when the handler fires and reads the input woof
	 */
	gettimeofday(&el->fielded,NULL);
	MAKE_EXTENDED_NAME(Oname,el->woof_name,"output");
	el->i_seqno = seq_no;
	/*
	 * copy the input element to the output woof
	 */
	o_seq_no = WooFPut(Oname,NULL,el);
	if(WooFInvalid(o_seq_no)) {
		fprintf(stdout,"couldn't write woof to %s\n",Oname);
		fflush(stdout);
	}

	return(1);
}

