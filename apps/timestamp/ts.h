#ifndef TS_H
#define TS_H

struct obj_stc
{
	long ts[16];
	size_t head;
	char woof[16][256];
};

typedef struct obj_stc TS_EL;

#endif

