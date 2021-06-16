#ifndef CSPOT_RUNSTAT_H
#define CSPOT_RUNSTAT_H

#include "woofc.h"

struct func_arg_stc
{
        int i;
        int j;
	unsigned long seq_no;
        int count;
        int sample_size;
        double alpha;
        char r[WOOFNAMESIZE];
	char stats[WOOFNAMESIZE];
	char rargs[WOOFNAMESIZE];
	char sargs[WOOFNAMESIZE];
	char kargs[WOOFNAMESIZE];
        char logfile[WOOFNAMESIZE];
};

typedef struct func_arg_stc FA;

#endif
