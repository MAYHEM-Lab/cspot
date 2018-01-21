#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mioregress.h"

/*
 * y values (predicted) in first column
 * x values in second column
 */
double *RegressMatrix(Array2D *matches)
{
	int i;
	int j;
	int c;
	Array2D *x;
	Array2D *b;
	Array2D *y;
	unsigned long size;
	double rsq;
	double rmse;
	double *coeff;

	x = MakeArray2D(2,matches->ydim);
	if(x == NULL) {
		return(NULL);
	}

	y = MakeArray1D(matches->ydim);
	if(y == NULL) {
		FreeArray2D(x);
		return(NULL);
	}

	for(i=0; i < matches->ydim; i++) {
		x->data[i*2+0] = 1.0; /* so we get a y-intercept */
		x->data[i*2+1] = matches->data[i*2+1]; /* x data in second column */
		y->data[i] = matches->data[i*2+0]; /* x data in first column */
	}

	/*
	 * destructive of x
	 */
	b = RegressMatrix2D(x,y);
	if(b == NULL) {
		fprintf(stderr,"regression failed\n");
		FreeArray2D(x);
		FreeArray2D(y);
		return(NULL);
	}

	coeff = (double *)malloc(2 * sizeof(double));
	if(coeff == NULL) {
		FreeArray2D(x);
		FreeArray2D(y);
		FreeArray2D(b);
		return(NULL);
	}

	/*
	 * intercept in first element, slope in second
	 */
	coeff[0] = b->data[0];
	coeff[1] = b->data[1];


	FreeArray2D(x);
	FreeArray2D(y);
	FreeArray2D(b);

	return(coeff);
}


	

	



