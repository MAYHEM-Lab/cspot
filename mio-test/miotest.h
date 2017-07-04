#ifndef MIOTEST
#define MIOTEST

#include "fsema.h"

struct marg_stc
{
	sema S;
	sema C;
	int turn;
	int counter;
};

typedef struct marg_stc MARG;

#endif
