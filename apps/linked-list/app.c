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

    clock_t start_time;
    clock_t end_time;

    if(argc == 2){
        num_ops_input = stoi(argv[1]);
    }else{
        num_ops_input = 1000;
    }

    LL_init(1, 10000, 10000, 10000);

    fp = fopen("../workload.txt", "r");

    fscanf(fp, "%d", &num_ops);
    start_time = clock();
    for(i = 0; i < num_ops; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        di.val = val;
        (op == 0) ? LL_delete(di) : LL_insert(di);
        if(i == (num_ops_input - 1)){
            break;
        }
    }
    end_time = clock();

    fclose(fp);


    //fprintf(stdout, "%d,%f\n", num_ops_input, (double) (end_time - start_time) / CLOCKS_PER_SEC);
    //fflush(stdout);

    //display_all_versions();

    return(0);
}

