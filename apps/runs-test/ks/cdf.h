#if !defined(CDF_H)
#define CDF_H

#include "hval_2.h"
#include "red_black_2.h"


int MakeCDF(void *data, int fields, double **cdf, int *size);
int MakeCDF_LE(void *data, int fields, double **cdf, int *size);
int CompressCDF(double *big, int bigsize, double **small, int *smallsize);
void PrintCDF(double *cdf, int size);

int FindCDFValue(double *cdf, int size, double frac);
int FindCDFFraction(double *cdf, int size, double value);

#endif

