#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "BST.h"
#include "DataItem.h"
#include "Data.h"

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    FILE *fp;
    int op;
    int val;
    int num_of_operations;
    DATA debug_data;

    WooFInit();
    BST_init(1, "AP", 1000, "DATA", 20000, 20000);   
    
    fp = fopen("../workload.txt", "r");
    fscanf(fp, "%d", &num_of_operations);

    for(i = 0; i < num_of_operations; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        di.val = val;
        (op == 0) ? BST_delete(di) : BST_insert(di);
    }
    fclose(fp);

    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        BST_preorder(i);
    }

    return(0);
}

