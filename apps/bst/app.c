#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "BST.h"
#include "DataItem.h"

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;

    WooFInit();
    BST_init(1, "AP", 100, "DATA", 100);   
    
    di.val = 'E';BST_insert(di);
    di.val = 'C';BST_insert(di);
    di.val = 'M';BST_insert(di);
    di.val = 'O';BST_insert(di);
    di.val = 'A';BST_insert(di);
    di.val = 'I';BST_insert(di);

    for(i = 5; i <= 6; ++i){
        BST_preorder(i);
    }

    BST_debug();

    return(0);
}

