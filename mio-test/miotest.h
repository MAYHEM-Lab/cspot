#ifndef MIOTEST
#define MIOTEST

#include <semaphore.h>
#include "fsema.h"

struct marg_stc
{
	sema S;
	sema C;
	int turn;
	int counter;
	sem_t l_sem;
};

typedef struct marg_stc MARG;

#endif
