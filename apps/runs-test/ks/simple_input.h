#ifndef SIMPLE_INPUT_H
#define SIMPLE_INPUT_H

#include <stdio.h>

#define DEFAULT_BLOCKSIZE (50000)
int InitDataSet(void **cookie, int fields);
void FreeDataSet(void *cookie);
void SetBlockSize(void *cookie, int size);
int LoadDataSet(char *fname, void *cookie);
void Rewind(void *cookie);
int SizeOf(void *cookie);
int WriteData(void *cookie, int fields, double *values);
int WriteEntry(void *cookie, double ts, double value);
int WriteEntry4(void *cookie, double a, double b, double c, double d);
int WriteEntry6(void *cookie, double a, double b, double c, 
			      double d, double e, double f);
int WriteEntry7(void *cookie, double a, double b, double c, 
			      double d, double e, double f, double g);
int WriteEntry10(void *cookie, double a, double b, double c, 
			      double d, double e, double f, double g,
			      double h, double i, double j);
int ReadData(void *cookie, int fields, double *values);
int ReadEntry(void *cookie, double *ts, double *value);
int ReadEntry4(void *cookie, double *a, double *b, double *c, double *d);
int ReadEntry5(void *cookie, double *a, double *b, double *c, double *d,
		double *e);
int ReadEntry7(void *cookie, double *a, double *b, double *c, double *d,
		double *e, double *f, double *g);
int ReadEntry10(void *cookie, double *a, double *b, double *c, 
			     double *d, double *e, double *f,
			     double *g, double *h, double *i, double *j);
int CopyDataSet(void *s, void *d, int count);

int GetFieldCount(char *fname);
double *GetFieldValues(void *cookie, int field);
int Current(void *cookie);
void PrintDataSetStats(FILE *fd, void *cookie);

#endif
