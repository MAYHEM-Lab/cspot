#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "woofc.h"
#include "mm_matrix.h"

int MM_ctrl_handler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	MM_CTRL *ctrl = (MM_CTRL *)ptr;
	MM_MUL mul;
	int row = ctrl->i;
	int col = ctrl->j;
	int dim = ctrl->dim;
	int i;
	unsigned long seq_no;


	// trigger dot product multiplies
	// A matrix runs across the row
	// B matrix runs down the column

	for(i=0; i < dim; i++) {
		mul.A_i = row;
		mul.A_j = i;
		mul.B_i = i;
		mul.B_j = col;
		mul.R_i = row;
		mul.R_j = col;
		mul.dim = dim;
		seq_no = WooFPut("MM_MUL_WooF","MM_mul_handler",&mul);
		if(seq_no == (unsigned long)-1) {
			fprintf(stderr,"MM_ctrl_handler failed at %d,%d (%d)\n",
				row,col,i);
		}
	}

	return(0);
}




