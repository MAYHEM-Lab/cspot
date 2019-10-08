#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "LinkedList.h"
#include "DataItem.h"

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    FILE *fp;
    int num_ops;
    int op;
    int val;

    LL_init(1, "AP", 10000, "DATA", 10000, 10000);

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

    return(0);
}

