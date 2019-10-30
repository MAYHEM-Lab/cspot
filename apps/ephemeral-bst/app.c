#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "BST.h"
#include "DataItem.h"
#include "Data.h"

#define LOG_SIZE_ENABLED 1
#define DISPLAY_ENABLED 0
#define GRANULAR_TIMING_ENABLED 0

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    unsigned long vs;
    FILE *fp;
    int op;
    int val;
    int num_of_operations;
    DATA debug_data;

#if GRANULAR_TIMING_ENABLED
    struct timeval ts_start;
    struct timeval ts_end;
#endif

    WooFInit();
    BST_init(20000, 20000, 20000);   
    
    fp = fopen("../workload.txt", "r");
    fscanf(fp, "%d", &num_of_operations);

    for(i = 0; i < num_of_operations; ++i){
        fscanf(fp, "%d %d", &op, &val);
        di.val = val;
        vs = VERSION_STAMP;
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_start, NULL);
#endif
        (op == 0) ? BST_delete(di) : BST_insert(di);
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_end, NULL);
        fprintf(stdout, "1,%lu,%d\n",
                (ts_end.tv_sec*1000000+ts_end.tv_usec) - (ts_start.tv_sec*1000000+ts_start.tv_usec), op);
        fflush(stdout);
#endif
#if LOG_SIZE_ENABLED
        log_size(i + 1);
#endif
#if DISPLAY_ENABLED
        if(vs != VERSION_STAMP){
            BST_preorder();
        }
#endif
    }
    fclose(fp);

    return 0;
}

