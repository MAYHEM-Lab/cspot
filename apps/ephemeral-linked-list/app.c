#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "LinkedList.h"
#include "DataItem.h"
#include "Helper.h"

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    unsigned long vs;
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
        vs = VERSION_STAMP;
        (op == 0) ? LL_delete(di) : LL_insert(di);
        if(VERSION_STAMP != vs){
            LL_print();
        }
        if(i == 30){
            break;
        }
    }

    fclose(fp);

    return(0);
}

