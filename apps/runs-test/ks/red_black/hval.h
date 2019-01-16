#ifndef HVAL_H
#define HVAL_H

union hval_un
{
	int i;
	int64_t i64;
	double d;
	void *v;
	char *s;
};

typedef union hval_un Hval;

#endif

