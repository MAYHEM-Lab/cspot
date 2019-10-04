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

    LL_init(1, "AP", 100, "DATA", 100, 100);

    di.val = 2;LL_insert(di);
    di.val = 5;LL_insert(di);
    di.val = 7;LL_insert(di);
    di.val = 9;LL_insert(di);
    di.val = 7;LL_delete(di);

    for(i = 1; i <= 5; ++i){
        LL_print(i);
    }

    return(0);
}

