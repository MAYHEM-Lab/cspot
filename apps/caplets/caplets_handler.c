#include "woofc.h"
#include "cspot_caplets.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int caplets_handler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
    EL *el = (EL*)ptr;
    printf("IN Caplets Handler: %s", el->string);
    return (1);
}