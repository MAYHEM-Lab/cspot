#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hw.h"
#include "boto3.h"

uint64_t stamp() {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return (uint64_t)spec.tv_sec * 1000 + (uint64_t)spec.tv_nsec / 1e6;
}

int main(int argc, char **argv)
{
	HW_EL el;

	el.client = stamp();
	srand(time(0));
	char key[32];
	sprintf(key, "%d", rand());
	
	put_item(key, (void *)&el);

	pthread_exit(NULL);
	return(0);
}

