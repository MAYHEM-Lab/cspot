#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "hval_2.h"
#include "red_black_2.h"

#include "simple_input.h"

int MakeCDF_LE(void *data, int fields, double **cdf, int *size)
{
	int count;
	double *lcdf;
	int i;
	int j;
	int k;
	RB *rb;
	RB *rp;
	RB *cp;
	double ts;
	double value;
	double *values;
	double next_value;
	double acc;
	double last_acc;

	Rewind(data);

	count = SizeOf(data);

	lcdf = (double *)malloc(count* 2 * sizeof(double));
	if(cdf == NULL)
	{
		exit(1);
	}

	values = (double *)malloc(fields*sizeof(double));
	if(values == NULL) {
		exit(1);
	}

	rb = RBInitD();

	if(rb == NULL)
	{
		exit(1);
	}

	while(ReadData(data,fields,values) != 0)
	{
		if(fields > 1) {
			value = values[1];
		} else {
			value = values[0];
		}
		RBInsertD(rb,value,(Hval)NULL);
	}
	

	last_acc = acc = 1.0/(double)count;
	i = 0;
	RB_FORWARD(rb,rp)
	{
		value = K_D(rp->key);
		if(rp->next != NULL) {
			next_value = K_D(rp->next->key);
		} else {
			next_value = -1.0;
		}

		j = 0;
		while(value == next_value) {
			last_acc = acc;
			acc += (1.0 / (double)count);
			rp = rp->next;
			if(rp == NULL) {
				break;
			}
			if(rp->next != NULL) {
				next_value = K_D(rp->next->key);
			}
			j++;
		}
		lcdf[i*2] = value;
		lcdf[i*2+1] = last_acc;
		i++;
		k=0;
		while(k < j) {
			lcdf[i*2] = value;
			lcdf[i*2+1] = last_acc;
			k++;
			i++;
		}

		if(rp == NULL) {
			break;
		}

		acc += (1.0 / (double)count);
		last_acc = acc;
	}

	RBDestroyD(rb);
	free(values);

	*cdf = lcdf;
	*size = count;

	return(1);
}

int MakeCDF(void *data, int fields, double **cdf, int *size)
{
	int count;
	double *lcdf;
	int i;
	RB *rb;
	RB *rp;
	double ts;
	double value;
	double last_value;
	double acc;
	double last_acc;
	double *values;

	Rewind(data);

	count = SizeOf(data);

	lcdf = (double *)malloc(count* 2 * sizeof(double));
	if(cdf == NULL)
	{
		exit(1);
	}

	rb = RBInitD();

	if(rb == NULL)
	{
		exit(1);
	}

	values = (double *)malloc(fields*sizeof(double));
	if(values == NULL) {
		exit(1);
	}

	while(ReadData(data,fields,values) != 0)
	{
		if(fields > 1) {
			value = values[1];
		} else {
			value = values[0];
		}
		RBInsertD(rb,value,(Hval)NULL);
	}
	
	acc = 1.0/(double)count;
	i = 0;
	last_value = -1.0;
	last_acc = acc;
	RB_FORWARD(rb,rp)
	{
		value = K_D(rp->key);
		lcdf[i*2] = value;
		if(value != last_value)
		{
			lcdf[i*2+1] = acc;
			last_acc = acc;
		}
		else
		{
			lcdf[i*2+1] = last_acc;
		}
		last_value = value;
		acc += (1.0 / (double)count);
		i++;
	}

	RBDestroyD(rb);
	free(values);

	*cdf = lcdf;
	*size = count;

	return(1);
}

void PrintCDF(double *cdf, int size)
{
	int i;
	for(i=0; i < size; i++)
	{
		fprintf(stdout,"%3.4f %3.4f\n",
			cdf[i*2],
			cdf[i*2+1]);
		fflush(stdout);
	}

	return;
}

int FindCDFValue(double *cdf, int size, double frac)
{
	int start;
	int end;
	int i;

	start = 0;
	end = size - 1;
	i = 0;

	while(!((frac >= cdf[i*2+1]) && (frac <= cdf[(i+1)*2+1])))
	{
		if((start == end) || (start == (end-1)))
		{
			i = start;
			break;
		}

		i = start + (end - start)/2;

		if(frac < cdf[i*2+1])
		{
			end = i;
		}
		else if(frac > cdf[i*2+1])
		{
			start = i;
		}
		else
		{
			break;
		}
	}

	if(frac >= cdf[(i+1)*2+1])
	{
		return(i+1);
	}
	else
	{
		return(i);
	}
}

int FindCDFFraction(double *cdf, int size, double value)
{
	int start;
	int end;
	int i;

	start = 0;
	end = size - 1;
	i = 0;

	while(!((value >= cdf[i*2]) && (value <= cdf[(i+1)*2])))
	{
		if((start == end) || (start == (end-1)))
		{
			i = start;
			break;
		}

		i = start + (end - start)/2;

		if(value < cdf[i*2])
		{
			end = i;
		}
		else if(value > cdf[i*2])
		{
			start = i;
		}
		else
		{
			break;
		}
	}

	if(value >= cdf[(i+1)*2])
	{
		return(i+1);
	}
	else
	{
		return(i);
	}
}


int CompressCDF(double *big, int bigsize, double **small, int *smallsize)
{
	int i;
	int j;
	int count;
	double *lsmall;

	count = 1;

	for(i=0; i < (bigsize-1); i++)
	{
		if(big[i*2] == big[(i+1)*2])
		{
			continue;
		}
		count++;
	}

	lsmall = (double *)malloc(count*sizeof(double)*2);
	if(lsmall == NULL)
	{
		fprintf(stderr,"CompressCDF no space\n");
		fflush(stderr);
		exit(1);
	}

	memset(lsmall,0,count*sizeof(double)*2);

	j = 0;
	for(i=0; i < count; i++)
	{
		lsmall[i*2] = big[j*2];
		lsmall[i*2+1] = big[j*2+1];
		while(big[j*2] == big[(j+1)*2])
		{
			/*
			 * pick up the larger fraction
			 */
			lsmall[i*2+1] = big[(j+1)*2+1];
			j++;
			if(j == bigsize) {
				break;
			}
		}
		j++;
		if(j == bigsize) {
			break;
		}
	}

	free(big);

	*smallsize = count;
	*small = lsmall;

	return(1);
}

#ifdef STANDALONE

int LE;

#define ARGS "f:L"

char *Usage = "cdf -f fname\n\
\t-L\ttoggle less-than-or-equal-to comparison off\n";

int
main(int argc, char *argv[])
{
	char fname[255];
	void *data;
	int c;
	int ierr;
	double *cdf;
	double *ncdf;
	int size;
	int nsize;
	int i;
	int fields;
	double *values;

	LE = 1;
	while((c = getopt(argc,argv,ARGS)) != EOF)
        {
		switch(c)
		{
			case 'f':
				strncpy(fname,optarg,sizeof(fname));
				break;
			case 'L':
				LE = 0;
				break;
			default:
				fprintf(stderr,"%s",Usage);
				fflush(stderr);
				exit(1);
				break;
		}
	}

	fields = GetFieldCount(fname);
	if(fields < 1) {
		fprintf(stderr,"no fields in %s\n",fname);
		exit(1);
	}

	ierr = InitDataSet(&data,fields);
	if(ierr < 0)
	{
		fprintf(stderr,"unable to open %s\n",fname);
		fflush(stderr);
		exit(1);
	}

	ierr = LoadDataSet(fname,data);
	if(ierr < 0)
	{
		fprintf(stderr,"unable to load %s\n",fname);
		fflush(stderr);
		exit(1);
	}

	if(LE)
	{
		MakeCDF_LE(data,fields,&cdf,&size);
	}
	else
	{
		MakeCDF(data,fields,&cdf,&size);
	}

	CompressCDF(cdf,size,&ncdf,&nsize);
	PrintCDF(ncdf,nsize);

	free(ncdf);

	exit(0);
}
#endif
