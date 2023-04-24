#ifndef PUT_TEST_H
#define PUT_TEST_H

#include <time.h>

struct put_test_stc
{
	char target_name[1024];
	char log_name[1024];
	unsigned long element_size;
	unsigned long history_size;
};

typedef struct put_test_stc PT_EL;

struct arg_stc
{
	PT_EL *el;
	char *target_name;
};

typedef struct arg_stc TARGS;

struct payload_stc
{
	unsigned long exp_seq_no;
	struct timeval tm;
};

typedef struct payload_stc PL;

struct exp_log_stc
{
	struct timeval tm;
	unsigned long exp_seq_no;
};

typedef struct exp_log_stc EX_LOG;

struct stress_stc
{
	char woof_name[116];
	unsigned long seq_no;
	struct timeval posted;
	struct timeval fielded;
};

typedef struct stress_stc ST_EL;

#define MAKE_EXTENDED_NAME(ename,wname,str) {\
        memset(ename,0,sizeof(ename));\
        sprintf(ename,"%s.%s",wname,str);\
}


#endif

