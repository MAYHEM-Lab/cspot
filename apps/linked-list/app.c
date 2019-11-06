#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "woofc.h"
#include "woofc-host.h"

#include "LinkedList.h"
#include "DataItem.h"
#include "CheckPointer.h"
#include "Helper.h"

#define LOG_SPACE_ENABLED 0
#define DISPLAY_ENABLED 0
#define GRANULAR_TIMING_ENABLED 1

char *WORKLOAD_SUFFIX;
unsigned long EXTRA_TIME;

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

void test_checkpointer(){

    CPRR *cprr;
    int i;

    cprr = CP_read(1);
    printf("%d\n", cprr->num_of_elements);
    for(i = 0; i < cprr->num_of_elements; ++i){
        printf("%s %lu\n", cprr->WooF_names[i], cprr->seq_nos[i]);
    }
}

void display_all_versions(){
    unsigned long i;

    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        LL_print(i);
    }
}

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
    FILE *fp;
    int num_ops;
    int num_ops_input;
    int op;
    int val;

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
        fprintf(stdout, "USAGE: %s <filename-relative-to-executable>\n", argv[0]);
        fflush(stdout);
        exit(1);
    }else{
        WORKLOAD_SUFFIX = get_workload_suffix(argv[1]);

#if GRANULAR_TIMING_ENABLED
        strcpy(TIMING_LOG_FILENAME, "../timing/persistent-linked-list-granular-time-");
        strcat(TIMING_LOG_FILENAME, WORKLOAD_SUFFIX);
        strcat(TIMING_LOG_FILENAME, ".log");

        fp_t = fopen(TIMING_LOG_FILENAME, "w");
        fclose(fp_t);
        fp_t = NULL;
#endif

#if LOG_SPACE_ENABLED
        strcpy(SPACE_LOG_FILENAME, "../space/persistent-linked-list-space-");
        strcat(SPACE_LOG_FILENAME, WORKLOAD_SUFFIX);
        strcat(SPACE_LOG_FILENAME, ".log");

        fp_s = fopen(SPACE_LOG_FILENAME, "w");
        fclose(fp_s);
        fp_s = NULL;
#endif
        
    }

    LL_init(1, 10000, 10000, 10000);
    fp = fopen(argv[1], "r");
    if(!fp){
        fprintf(stdout, "could not open workload file\n");
        fflush(stdout);
        exit(1);
    }
    fscanf(fp, "%d", &num_ops);

    for(i = 0; i < num_ops; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        di.val = val;
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_start, NULL);
#endif
        (op == 0) ? LL_delete(di) : LL_insert(di);
#if GRANULAR_TIMING_ENABLED
        gettimeofday(&ts_end, NULL);
        fp_t = fopen(TIMING_LOG_FILENAME, "a");
        if(fp_t != NULL){
            fprintf(fp_t, "1,%lu,%d\n",
                (ts_end.tv_sec*1000000+ts_end.tv_usec)-(ts_start.tv_sec*1000000+ts_start.tv_usec)-EXTRA_TIME, op);
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
    }

    fclose(fp);

#if DISPLAY_ENABLED
    display_all_versions();
#endif

    return(0);
}

