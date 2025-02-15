#include "hw.h"
#include "woofc.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

int cjkHlw(WOOF* wf, unsigned long seq_no, void* ptr) {
    	HW_EL* el = (HW_EL*)ptr;
	int err;
    	fprintf(stdout, "[CJK] cjkHlw: triggered ");
    	fprintf(stdout, "at %lu with string: %s\n", seq_no, el->string);
    	fflush(stdout);
	long long parsed_time = atoll(el->string);
	struct timespec ts;
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
   	if (err == -1) {
        	printf("[CJK] cjkHlw: Error getting the current time.\n");
        	return 1;
    	}
	long long now = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
	long tdiff = now - parsed_time;
	double diff_ms = (double)tdiff / 1000000.0; //convert to milliseconds
        printf("[CJK] cjkHlw: time diff in milliseconds: %.6f\n",diff_ms);
    	return 0;
}
