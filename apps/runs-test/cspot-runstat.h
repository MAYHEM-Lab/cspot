#ifndef CSPOT_RUNSTAT_H
#define CSPOT_RUNSTAT_H

#include "woofc.h"

struct func_arg_stc
{
        int i;
        int j;
	unsigned long ndx;
        int count;
        int sample_size;
        double alpha;
        char r[WOOFNAMESIZE];
	char stats[WOOFNAMESIZE];
        char logfile[WOOFNAMESIZE];
};

typedef struct func_arg_stc FA;

#endif
