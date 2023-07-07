#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "woofc.h"

#include "mm_matrix.h"

char *Usage = "mm_print_result\n";

int main(int argc, char **argv)
{
	int c;
	MM_RES res;
	int dim;
	int count;
	unsigned long seq_no;
	int err;
	double *array;
	int *found;
	int i;
	int j;
	double earliest;
	double latest;
	double duration;

	// init the CSPOT environment
	WooFInit();

	seq_no = WooFGetLatestSeqno("MM_RES_WooF");
	if(seq_no == (unsigned long)-1) {
		fprintf(stderr,"couldn't get latest seqno of result\n");
		exit(1);
	}
	// get the tail
//	err = WooFGet("MM_RES_WooF",&res,seq_no);
	err = WooFGet("MM_RES_WooF",&res,1);
	if(err < 0) {
		fprintf(stderr,"mm_print_result: could not get tail of result at %lu\n",
			seq_no);
		exit(1);
	}

	dim = res.dim;

	array = (double *)malloc(dim*dim*sizeof(double));
	if(array == NULL) {
		exit(1);
	}

	found = (int *)malloc(dim*dim*sizeof(int));
	if(found == NULL) {
		exit(1);
	}
	for(i=0; i < dim*dim; i++) {
		found[i] = 0;
	}

	array[res.R_i*dim+res.R_j] = res.val;
	found[res.R_i*dim+res.R_j] = 1;
	earliest = res.start;
	latest = res.end;
	
	count = 1;
//	seq_no--;
	seq_no = 1;
	while(count < (dim*dim)) {
		err = WooFGet("MM_RES_WooF",&res,seq_no);
		if(err < 0) {
			fprintf(stderr,"mm_print_result: could not get result at %lu after %d\n",
				seq_no,count);
			exit(1);
		}

		//
		// prevents an additional scan in MM_add_handler to determine if
		// R[i,j] has been produced already
		//
		if(found[res.R_i*dim+res.R_j] == 1) {
//			seq_no--;
			seq_no++;
			continue;
		}
		array[res.R_i*dim+res.R_j] = res.val;
		found[res.R_i*dim+res.R_j] = 1;
		if(res.start < earliest) {
			earliest = res.start;
		}

		if(res.end > latest) {
			latest = res.end;
		}
//		seq_no--;
		seq_no++;
		count++;
	}

	if(count != (dim*dim)) {
		fprintf(stderr,"found %d of %d values in result\n",count,dim*dim);
		exit(1);
	}

	for(i=0; i < dim; i++) {
		printf("row %d : ",i);
		for(j=0; j < dim; j++) {
			printf("%4.0f ",array[i*dim+j]);
		}
		printf("\n");
	}

	duration = DURATION(earliest,latest) * 1000;

	printf("total time: %f ms, avg: %f ms\n",
		duration,
		duration/(double)(dim*dim));

	free(array);

	exit(0);
}


