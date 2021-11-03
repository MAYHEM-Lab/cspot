#ifndef TS_H
#define TS_H

#include <stdint.h>

struct obj_stc
{
	uint64_t ts[16];
	uint8_t head;
	uint8_t stop;
	char woof[16][256];
	uint8_t repair;
	uint8_t again;
};

typedef struct obj_stc TS_EL;

#endif
