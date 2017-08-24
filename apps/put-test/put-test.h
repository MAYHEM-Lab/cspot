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

struct payload_stc
{
	unsigned long exp_seq_no;
};

typedef struct payload_stc PL;

struct exp_log_stc
{
	struct timeval tm;
	unsigned long exp_seq_no;
};

typedef struct exp_log_stc EX_LOG;


#endif

