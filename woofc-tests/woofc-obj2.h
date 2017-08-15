#ifndef WOOF_OBJ2
#define WOOF_OBJ2

struct obj2_stc
{
	unsigned long counter;
	unsigned long max;
	char next_woof[1024];
	char next_woof2[1024];
};

typedef struct obj2_stc OBJ2_EL;

#endif

