#ifndef PUT_TEST_H
#define PUT_TEST_H

#include <time.h>
#include <stdint.h>

struct put_test_stc
{
	char target_name[1024];
	char log_name[1024];
	unsigned long element_size;
	unsigned long history_size;
};

typedef struct put_test_stc PT_EL;

struct payload_stc
{
	uint32_t exp_seq_no;
	struct timeval tm;
};

typedef struct payload_stc PL;

struct exp_log_stc
{
	struct timeval tm;
	uint32_t exp_seq_no;
};

typedef struct exp_log_stc EX_LOG;

struct stress_stc
{
	char woof_name[16];
	uint32_t seq_no;
	uint64_t posted;
	uint64_t fielded;
};

typedef struct stress_stc ST_EL;

#define MAKE_EXTENDED_NAME(ename,wname,str) {\
        memset(ename,0,sizeof(ename));\
        sprintf(ename,"%s.%s",wname,str);\
}


#endif

