#ifndef HVAL_H
#define HVAL_H

union hval_un
{
	int i;
	double d;
	void *v;
	char *s;
};

typedef union hval_un Hval;

#endif

