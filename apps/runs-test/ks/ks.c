#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cdf.h"
#include "simple_input.h"

int yan()
{
	printf("yan\n");
	return(0);
}

double KS(void *s1, int f1, void *s2, int f2)
{
	double *tcdf;
	int tsize;
	double *cdf1;
	int size1;
	double *cdf2;
	int size2;
	int err;
	int i;
	int j;
	int last_j;
	double max;
	double diff;
	int done;
	double value1;
	double value2;
	

	err = MakeCDF_LE(s1,f1,&tcdf,&tsize);
	if(err <= 0) {
		fprintf(stderr,"KS: couldn't make cdf for s1\n");
		fflush(stderr);
		return(-1.0);
	}

	err = CompressCDF(tcdf,tsize,&cdf1,&size1);
	if(err <= 0) {
		fprintf(stderr,"KS: couldn't compress cdf for s1\n");
		fflush(stderr);
		return(-1.0);
	}
#if 0
cdf1 = tcdf;
size1 = tsize;
#endif

	err = MakeCDF_LE(s2,f2,&tcdf,&tsize);
	if(err <= 0) {
		fprintf(stderr,"KS: couldn't make cdf for s2\n");
		fflush(stderr);
		free(cdf1);
		return(-1.0);
	}
	err = CompressCDF(tcdf,tsize,&cdf2,&size2);
	if(err <= 0) {
		fprintf(stderr,"KS: couldn't compress cdf for s2\n");
		fflush(stderr);
		return(-1.0);
	}
#if 0
cdf2 = tcdf;
size2 = tsize;
#endif

	value1 = cdf1[i*0+0];
	value2 = cdf2[i*0+0];

	/*
	 * swap them to get the first comparison right
	 */
	if(value2 > value1) {
		tsize = size1;
		tcdf = cdf1;
		size1 = size2;
		cdf1 = cdf2;
		size2 = tsize;
		cdf2 = tcdf;
	}

	max = 0;
	j = 0;
	done = 0;
	for(i=0; i < size1; i++)
	{
		value1 = cdf1[i*2+0];
		value2 = cdf2[j*2+0];

		if(value1 == value2) {
			diff = fabs(cdf1[i*2+1] - cdf2[j*2+1]);
			if(diff > max) {
				max = diff;
			}
			j++;
			if(j == size2) {
				done = 1;
				break;
			}
			continue;
		}
		if(value2 > value1) {
			continue;
		}
		j++;
		if(j == size2) {
			done = 1;
			break;
		}
		value2 = cdf2[j*2+0];
		while(value2 < value1)
		{
			j++;
			if(j == size2) {
				done = 1;
				break;
			}
			value2 = cdf2[j*2+0];
		}
		if(done == 1) {
			break;
		}
		/*
		 * here, we have the place where value2 crosses value1 from
		 * < to >
		 *
		 * compute the diff of the CDF fractions
		 */
//		diff = fabs(cdf1[i*2+1] - cdf2[last_j*2+1]);
		diff = fabs(cdf1[i*2+1] - cdf2[j*2+1]);
		if(diff > max) {
			max = diff;
		}

		if(max > 1.0) {
			yan();
		}
	}

	if(done == 1) {
		/*
		 * now find the value in cdf1 that is the supremum to
		 * last value in cdf2
		 */
		while((i >= 0) && (cdf1[i*2+0] >= value2)) {
			i--;
		}
		diff = fabs(cdf1[i*2+1] - 1.0);
		if(diff > max) {
			max = diff;
		}
	} else {
		while((j >= 0) && (cdf2[j*2+0] >= value1)) {
			j--;
		}
		diff = fabs(1.0 - cdf2[j*2+1]);
		if(diff > max) {
			max = diff;
		}
	}


	free(cdf1);
	free(cdf2);

	return(max);
		
}

double KSCritical(double alpha, double m, double n)
{
	double critical;

	critical = sqrt(-0.5 * log(alpha/2.0)) *
                sqrt((n + m) / (n*m));

	return(critical);
}

#ifdef STANDALONE

#define ARGS "f:g:"
char *Usage = "ks -f sample_file_1\n\
\t-g sample_file_2\n";

char Fname[255];
char Gname[255];

int main(int argc, char **argv)
{
	int c;
	int f1;
	int f2;
	void *s1;
	void *s2;
	int err;
	double ks;

	while((c = getopt(argc,argv,ARGS)) != EOF)
	{
		switch(c)
		{
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'g':
				strncpy(Gname,optarg,sizeof(Gname));
				break;
			default:
				fprintf(stderr,"usage: %s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"need to specify sample file 1\n");
		fprintf(stderr,"usage: %s",Usage);
		exit(1);
	}

	if(Gname[0] == 0) {
		fprintf(stderr,"need to specify sample file 2\n");
		fprintf(stderr,"usage: %s",Usage);
		exit(1);
	}

	f1 = GetFieldCount(Fname);
	f2 = GetFieldCount(Gname);

	err = InitDataSet(&s1,f1);
	if(err <= 0) {
		fprintf(stderr,"couldn't init %s\n",Fname);
		exit(1);
	}

	err = InitDataSet(&s2,f2);
	if(err <= 0) {
		fprintf(stderr,"couldn't init %s\n",Gname);
		exit(1);
	}

	err = LoadDataSet(Fname,s1);
	if(err <= 0) {
		fprintf(stderr,"couldn't load %s\n",Fname);
		exit(1);
	}

	err = LoadDataSet(Gname,s2);
	if(err <= 0) {
		fprintf(stderr,"couldn't load %s\n",Gname);
		exit(1);
	}

	ks = KS(s1,f1,s2,f2);

	if(ks < 0) {
		fprintf(stderr,"ks failed\n");
		exit(1);
	}

	printf("ks: %f\n",ks);

	FreeDataSet(s1);
	FreeDataSet(s2);

	return(0);

}

#endif

