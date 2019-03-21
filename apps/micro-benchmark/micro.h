#ifndef MICRO_H
#define MICRO_H

#include <stdint.h>

struct obj_stc
{
	char payload[1024];
};

typedef struct obj_stc MICRO_EL;

#endif

