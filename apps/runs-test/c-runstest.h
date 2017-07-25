#ifndef C_RUNSTEST_H
#define C_RUNSTEST_H

#ifndef HAS_WOOF
double RunsStat(double *v, int N);
#else
double RunsStat(char *v, int N);
#endif


#endif

