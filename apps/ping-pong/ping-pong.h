#ifndef PP_H
#define PP_H

struct obj_pp_stc
{
	unsigned long counter;
	unsigned long max;
	char next_woof[1024];
	char next_woof2[1024];
};

typedef struct obj_pp_stc PP_EL;

#endif

