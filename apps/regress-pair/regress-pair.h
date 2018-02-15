#ifndef REGRESS_PAIR_H
#define REGRESS_PAIR_H

#include "hval.h"
#include <string.h>
#include <time.h>

/*
 * parameters for regression
 */
struct regress_index_stc
{
	int count_back;
	int max_lags;
};

struct regress_value_stc
{
	char woof_name[4096];
	char series_type;
	char type;
	Hval value;
	char ip_addr[25];
	unsigned long seq_no;
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned int pad_1;
	unsigned int pad_2;
};

struct regress_coeff_stc
{
	char woof_name[4096];
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned int pad_1;
	unsigned int pad_2;
	double slope;
	double intercept;
	double measure;
	int lags;
	unsigned long earliest_seq_no;
};


typedef struct regress_index_stc REGRESSINDEX;
typedef struct regress_value_stc REGRESSVAL;
typedef struct regress_coeff_stc REGRESSCOEFF;

#define MAKETS(ts,rv) (ts = (double)ntohl((rv)->tv_sec)+(double)(ntohl((rv)->tv_usec) / 1000000.0))

#define MAKE_EXTENDED_NAME(ename,wname,str) {\
        memset(ename,0,sizeof(ename));\
        sprintf(ename,"%s.%s",wname,str);\
}

#define MAXLAGS (12)

#endif
