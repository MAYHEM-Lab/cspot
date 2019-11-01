#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "BST.h"
#include "DataItem.h"
#include "Data.h"

#define LOG_SPACE_ENABLED 0
#define DISPLAY_ENABLED 0
#define GRANULAR_TIMING_ENABLED 0

char *WORKLOAD_SUFFIX;

char *get_workload_suffix(char *filename){
    char *suffix;
    int suffix_size;
    int j;
    
    suffix_size = 3;
    suffix = (char *)malloc((suffix_size + 1) * sizeof(char));
    memset(suffix, 0, (suffix_size + 1));

    for(; *filename; filename++){
        if(*filename == '-'){
            for(j = 0; j < suffix_size; ++j){
                suffix[j] = *(filename + j + 1);
            }
            return suffix;
        }
    }
}

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

    FILE *fp_t;
    char TIMING_LOG_FILENAME[255];
#endif

#if LOG_SPACE_ENABLED
    FILE *fp_s;
    char SPACE_LOG_FILENAME[255];
#endif

    if(argc != 2){
        fprintf(stdout, "please provide workload file\n");
        fprintf(stdout, "USAGE: %s <workload-filename>\n", argv[0]);
        exit(1);
    }else{
        WORKLOAD_SUFFIX = get_workload_suffix(argv[1]);

#if GRANULAR_TIMING_ENABLED
        strcpy(TIMING_LOG_FILENAME, "../timing/ephemeral-binary-search-tree-granular-time-");
        strcat(TIMING_LOG_FILENAME, WORKLOAD_SUFFIX);
        strcat(TIMING_LOG_FILENAME, ".log");

        fp_t = fopen(TIMING_LOG_FILENAME, "w");
        fclose(fp_t);
        fp_t = NULL;
#endif

#if LOG_SPACE_ENABLED
        strcpy(SPACE_LOG_FILENAME, "../space/ephemeral-binary-search-tree-space-");
        strcat(SPACE_LOG_FILENAME, WORKLOAD_SUFFIX);
        strcat(SPACE_LOG_FILENAME, ".log");

        fp_s = fopen(SPACE_LOG_FILENAME, "w");
        fclose(fp_s);
        fp_s = NULL;
#endif
        
    }

    WooFInit();
    BST_init(20000, 20000, 20000);   
    
    fp = fopen(argv[1], "r");
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
        fp_t = fopen(TIMING_LOG_FILENAME, "a");
        if(fp_t != NULL){
            fprintf(fp_t, "1,%lu,%d\n",
                    (ts_end.tv_sec*1000000+ts_end.tv_usec) - (ts_start.tv_sec*1000000+ts_start.tv_usec), op);
        }
        fflush(fp_t);
        fclose(fp_t);
        fp_t = NULL;
#endif
#if LOG_SPACE_ENABLED
        fp_s = fopen(SPACE_LOG_FILENAME, "a");
        log_size(i + 1, fp_s);
        fflush(fp_s);
        fclose(fp_s);
        fp_s = NULL;
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

