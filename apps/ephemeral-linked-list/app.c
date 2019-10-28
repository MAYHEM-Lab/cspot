#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "woofc.h"
#include "woofc-host.h"

#include "LinkedList.h"
#include "DataItem.h"
#include "Helper.h"

#define TIMING_ENABLED 0
#define DISPLAY_ENABLED 0
#define LOG_SIZE_ENABLED 0
#define GRANULAR_TIMING_ENABLED 1

int stoi(char *str){
    int val;
    int multiplier;

    val = 0;
    multiplier = 10;
    while(*str){
        val = val + (*str - '0');
        val *= multiplier;
        str++;
    }
    val /= multiplier;

    return val;
}


int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    unsigned long vs;
    FILE *fp;
    int num_ops;
    int num_ops_input;
    int op;
    int val;

#if TIMING_ENABLED
    clock_t start_time;
    clock_t end_time;
#endif
#if GRANULAR_TIMING_ENABLED
    struct timeval ts_start;
    struct timeval ts_end;
#endif

    if(argc == 2){
        num_ops_input = stoi(argv[1]);
    }else{
        num_ops_input = 1000;
    }

    LL_init(1, 10000, 10000, 10000);

    fp = fopen("../workload.txt", "r");

    fscanf(fp, "%d", &num_ops);

#if TIMING_ENABLED
    start_time = clock();
#endif

    for(i = 0; i < num_ops; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        di.val = val;
        vs = VERSION_STAMP;
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_start, NULL);
#endif
        (op == 0) ? LL_delete(di) : LL_insert(di);
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_end, NULL);
        fprintf(stdout, "1,%lu,%d\n", 
                (uint64_t)((ts_end.tv_sec*1000000+ts_end.tv_usec)-(ts_start.tv_sec*1000000+ts_start.tv_usec)), op);
#endif
#if LOG_SIZE_ENABLED
        log_size(i + 1);
#endif
#if DISPLAY_ENABLED
        LL_print();
#endif
        if(i == num_ops_input - 1){
            break;
        }
    }

#if TIMING_ENABLED
    end_time = clock();
#endif

#if TIMING_ENABLED
    fprintf(stdout, "%d,%f\n", num_ops_input, (double) (end_time - start_time) / CLOCKS_PER_SEC);
#endif

    fclose(fp);


    return(0);
}

