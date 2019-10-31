#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"

#include "BST.h"
#include "DataItem.h"
#include "Data.h"

#define LOG_SIZE_ENABLED 0
#define DISPLAY_ENABLED 1
#define GRANULAR_TIMING_ENABLED 0

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    FILE *fp;
    int op;
    int val;
    int num_of_operations;
    DATA debug_data;

#if GRANULAR_TIMING_ENABLED
    struct timeval ts_start;
    struct timeval ts_end;
#endif

    if(argc != 2){
        fprintf(stdout, "USAGE: %s <filename-relative-to-executable>\n", argv[0]);
        fflush(stdout);
        exit(1);
    }

    WooFInit();
    BST_init(1, "AP", 1000, "DATA", 20000, 20000);   
    
    fp = fopen(argv[1], "r");
    if(!fp){
        fprintf(stdout, "could not open workload file\n");
        fflush(stdout);
        exit(1);
    }
    fscanf(fp, "%d", &num_of_operations);

    for(i = 0; i < num_of_operations; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        di.val = val;
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_start, NULL);
#endif
        (op == 0) ? BST_delete(di) : BST_insert(di);
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_end, NULL);
        fprintf(stdout, "1,%lu,%d\n",
                (uint64_t)((ts_end.tv_sec*1000000+ts_end.tv_usec)-(ts_start.tv_sec*1000000+ts_start.tv_usec)), op);
        fflush(stdout);
#endif
#if LOG_SIZE_ENABLED
        log_size(i + 1);
#endif
    }
    fclose(fp);

#if DISPLAY_ENABLED
    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        if(i == 1000){
        BST_preorder(i);
        }
    }
#endif

    return(0);
}

