#ifndef MIOTEST
#define MIOTEST

#include <semaphore.h>
#include "lsema.h"

struct marg_stc
{
	sema S;
	sema C;
	int turn;
	int counter;
};

typedef struct marg_stc MARG;

#endif
