#ifndef HW_H
#define HW_H

struct obj_stc
{
	uint64_t client;
	uint64_t server;
};

typedef struct obj_stc HW_EL;

uint64_t stamp();

#endif

