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

    WooFInit();
    BST_init(1, "AP", 100, "DATA", 100);   
    
    di.val = 'E';BST_insert(di);
    di.val = 'C';BST_insert(di);
    di.val = 'M';BST_insert(di);

    BST_preorder(1);
    BST_preorder(2);
    BST_preorder(3);

    BST_debug();

    return(0);
}

