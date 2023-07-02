#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "woofc.h"

#include "mm_matrix.h"

char *Usage = "mm_init -d dimension\n";
#define ARGS "d:"

int main(int argc, char **argv)
{
	int c;
	int dim = 0;
	int i;
	int j;
	MM_MAT m;
	MM_CTRL ctrl;
	int count;
	unsigned long seq_no;
	int err;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'd':
				dim = atoi(optarg);
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);	
		}
	}

	if(dim == 0) {
		fprintf(stderr,"dim must be greater than zero\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	// init the CSPOT environment
	WooFInit();

	// matrix woof to hold the A matrix
	err = WooFCreate("MM_A_WooF",sizeof(MM_MAT),dim*dim+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for A WooF failed\n");
		exit(1);
	}

	// matrix woof to hold the B matrix
	err = WooFCreate("MM_B_WooF",sizeof(MM_MAT),dim*dim+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for B WooF failed\n");
		exit(1);
	}

	// multiply woof to hold the dot product elements
	err = WooFCreate("MM_MUL_WooF",sizeof(MM_MUL),dim*dim*dim+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for multiply WooF failed\n");
		exit(1);
	}

	// add woof to hold the dot product terms
	err = WooFCreate("MM_ADD_WooF",sizeof(MM_ADD),dim*dim*dim+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for add WooF failed\n");
		exit(1);
	}

	// result woof to hold the results
	err = WooFCreate("MM_RES_WooF",sizeof(MM_RES),dim*dim*dim+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for res WooF failed\n");
		exit(1);
	}

	// ctrl woof to trigger computations
	err = WooFCreate("MM_CTRL_WooF",sizeof(MM_CTRL),dim*dim+1);
	if(err < 0) {
		fprintf(stderr,"WooFCreate for ctrl WooF failed\n");
		exit(1);
	}

	// load up the A matrix
	count = 0;
	m.dim = dim;
	for(i=0; i < dim; i++) {
		for(j=0; j < dim; j++) {
			m.i = i;
			m.j = j;
			m.val = count;
			count++;
			seq_no = WooFPut("MM_A_WooF",NULL,&m);
			if(seq_no == (unsigned long)-1) {
				fprintf(stderr,
					"put of %d at %d,%d in A failed\n",
						count,i,j);
			}
	
		}
	}

	// load up the B matrix
	count = 0;
	m.dim = dim;
	for(i=0; i < dim; i++) {
		for(j=0; j < dim; j++) {
			m.i = i;
			m.j = j;
			m.val = count;
			count++;
			seq_no = WooFPut("MM_B_WooF",NULL,&m);
			if(seq_no == (unsigned long)-1) {
				fprintf(stderr,
					"put of %d at %d,%d in B failed\n",
						count,i,j);
			}
	
		}
	}

	// trigger the matrix multiply
	ctrl.dim = dim;
	for(i=0; i < dim; i++) {
		for(j=0; j < dim; j++) {
			ctrl.i = i;
			ctrl.j = j;
			ctrl.dim = dim;
			seq_no = WooFPut("MM_CTRL_WooF",
				"MM_ctrl_handler",
				&ctrl);
			if(seq_no == (unsigned long)-1) {
				fprintf(stderr,
					"put at %d,%d of %d in ctrl failed\n", i,j,count);
			}
	
		}
	}

	exit(0);
}

	
	

	

