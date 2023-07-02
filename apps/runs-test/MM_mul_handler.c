#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "woofc.h"
#include "mm_matrix.h"

int MM_mul_handler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	MM_MUL *mul = (MM_MUL *)ptr;
	MM_ADD add;
	int err;
	MM_MAT a;
	MM_MAT b;
	unsigned long seq_no;
	unsigned long next_seq_no;

	// pass through values needed by add handler
	add.R_i = mul->R_i;
	add.R_j = mul->R_j;
	add.A_i = mul->A_i;
	add.A_j = mul->A_j;
	add.B_i = mul->B_i;
	add.B_j = mul->B_j;
	add.dim = mul->dim;


	// scan A matrix backwards looking for i,j value
	next_seq_no = WooFGetLatestSeqno("MM_A_WooF");
	while(next_seq_no != (unsigned long)-1) {
		err = WooFGet("MM_A_WooF",&a,next_seq_no);
		if(err < 0) {
			fprintf(stderr,"MM_mul_handler err at %lu in A\n",
				next_seq_no);
			return(1);
		}
		if((a.i == mul->A_i) && (a.j == mul->A_j)) {
			// found it!
			break;
		}
		next_seq_no--;
	}
	if(next_seq_no == (unsigned long)-1) {
		fprintf(stderr,"MM_mul_handler no A value for %d,%d\n",
			mul->A_i,mul->A_j);
		return(1);
	}

	// a is set, now find b

	// scan B matrix backwards looking for i,j value
	next_seq_no = WooFGetLatestSeqno("MM_B_WooF");
	while(next_seq_no != (unsigned long)-1) {
		err = WooFGet("MM_A_WooF",&b,next_seq_no);
		if(err < 0) {
			fprintf(stderr,"MM_mul_handler err at %lu in B\n",
				next_seq_no);
			return(1);
		}
		if((b.i == mul->B_i) && (b.j == mul->B_j)) {
			// found it!
			break;
		}
		next_seq_no--;
	}
	if(next_seq_no == (unsigned long)-1) {
		fprintf(stderr,"MM_mul_handler no B value for %d,%d\n",
			mul->B_i,mul->B_j);
		return(1);
	}

	// here we have the A and B elements, multiply them and put to the add
	// woof

	add.val = a.val * b.val;

	// trigger an attempt to compute the dot product for R_i,R_j in
	// the add handler
	seq_no = WooFPut("MM_ADD_WooF","MM_add_handler",&add);
	if(seq_no == (unsigned long)-1) {
		fprintf(stderr,
			"MM_mul_handler error putting to add handler\n");
	}
	return(0);
}
	
