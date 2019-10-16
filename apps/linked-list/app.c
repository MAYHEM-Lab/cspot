#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "LinkedList.h"
#include "DataItem.h"
#include "CheckPointer.h"
#include "Helper.h"

void test_checkpointer(){

    CPRR *cprr;
    int i;
    char *WooF_names[] = {
        "aaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbb"
    };
    unsigned long seq_nos[] = {5678, 3412};

    CP_write(2, WooF_names, seq_nos);
    cprr = CP_read(1);
    printf("%d\n", cprr->num_of_elements);
    for(i = 0; i < cprr->num_of_elements; ++i){
        printf("%s %lu\n", cprr->WooF_names[i], cprr->seq_nos[i]);
    }
}

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    FILE *fp;
    int num_ops;
    int op;
    int val;

    LL_init(1, 10000, 10000, 10000);

    fp = fopen("../workload.txt", "r");

    fscanf(fp, "%d", &num_ops);
    for(i = 0; i < num_ops; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        di.val = val;
        (op == 0) ? LL_delete(di) : LL_insert(di);
    }

    fclose(fp);

    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        LL_print(i);
    }

    //test_checkpointer();

    return(0);
}

