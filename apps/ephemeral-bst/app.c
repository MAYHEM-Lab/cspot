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
    unsigned long vs;
    FILE *fp;
    int op;
    char val;
    int num_of_operations;
    int line_number;
    DATA debug_data;

    WooFInit();
    BST_init(20000, 20000, 20000);   
    
    fp = fopen("../workload.txt", "r");
    fscanf(fp, "%d", &num_of_operations);

    for(i = 0, line_number = 2; i < num_of_operations; ++i, ++line_number){
        fscanf(fp, "%d %c", &op, &val);
        di.val = val;
        vs = VERSION_STAMP;
        (op == 0) ? BST_delete(di) : BST_insert(di);
        if(vs != VERSION_STAMP){
            BST_preorder();
        }
    }
    fclose(fp);

    return 0;
}

