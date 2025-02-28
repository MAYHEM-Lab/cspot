#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "woofc.h"
#include "mm_matrix.h"

int MM_add_handler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	MM_ADD *add = (MM_ADD *)ptr;
	MM_ADD add_el; // individual element
	MM_RES res;
	int err;
	unsigned long seq_no;
	unsigned long next_seq_no;
	double *dot_prod;
	int dot_count;
	double acc;
	int i;
	double now;

	next_seq_no = WooFGetLatestSeqno("MM_ADD_WooF");
	if(next_seq_no == (unsigned long)-1) {
		fprintf(stderr,"MM_add_handler: latest seqno failed\n");
		return(1);
	}

	// scan add woof looking to see if we have all of the values
	// for a dot product

	dot_prod = (double *)malloc(add->dim*sizeof(double));
	if(dot_prod == NULL) {
		return(1);
	}
	dot_count = 0;

	while(dot_count < add->dim) {
		err = WooFGet("MM_ADD_WooF",&add_el,next_seq_no);
		if(err < 0) {
			fprintf(stderr,"MM_add_handler failed to get at %lu\n",
				next_seq_no);
			free(dot_prod);
			return(1);
		}

		// this will not work if there are duplicates.  That would require that
		// we keep a list of elements we have seen so we can could ignore
		// the duplicates
		if((add->R_i == add_el.A_i) &&
		   (add->R_j == add_el.B_j)) {
			// found one
			dot_prod[dot_count] = add_el.val;
			dot_count++;
		}
		if(dot_count == add->dim) {
			break;
		}
		next_seq_no--;
	}

	// if we don't find all of the values, bail out
	if(next_seq_no == 0) {
		free(dot_prod);
		return(0);
	}

	// otherwise, compute the dot product and store it
	acc = 0;
	for(i=0; i < add->dim; i++) {
		acc += dot_prod[i];
	} 

	res.R_i = add->R_i; 
	res.R_j = add->R_j; 
	res.val = acc;
	res.dim = add->dim;
	res.start = add->start;
	STOPCLOCK(&now);
	res.end = now;
	seq_no = WooFPut("MM_RES_WooF",NULL,&res);
	if(seq_no == (unsigned long)-1) {
		fprintf(stderr,"MM_add_handler: couldn't write result\n");
	}
	free(dot_prod);
	return(0);

}
	
