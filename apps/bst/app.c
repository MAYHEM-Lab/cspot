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
    
    BST_init(1, "AP", 100, "DATA", 100);   
    
    di.val = 'E';BST_insert(di);
    
    pthread_exit(NULL);
    return(0);
}

