#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <time.h>
#include <sys/time.h>

struct ex_stc
{
	char woof_name[256];
	struct timeval posted;
	struct timeval fielded;
	unsigned long i_seqno;
};

typedef struct ex_stc EX_EL;

#define MAKE_EXTENDED_NAME(ename,wname,str) {\
        memset(ename,0,sizeof(ename));\
        sprintf(ename,"%s.%s",wname,str);\
}


#endif

