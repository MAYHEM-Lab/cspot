#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "ledger.h"

int main(int argc, char **argv)
{
    unsigned long seq;
    unsigned long i;
    int err;
    LEDGER_ENTRY entry;

    WooFInit();

    seq = WooFGetLatestSeqno(LEDGER_WOOF_NAME);
    if (WooFInvalid(seq))
    {
        fprintf(stderr, "Couldn't get the latest seqno of %s\n", LEDGER_WOOF_NAME);
        fflush(stderr);
        exit(1);
    }
    printf("%lu entries in ledger\n", seq);
    for (i = 1; i <= seq; i++)
    {
        err = WooFGet(LEDGER_WOOF_NAME, &entry, i);
        if (err < 0)
        {
            fprintf(stderr, "Couldn't read %s[%lu]\n", LEDGER_WOOF_NAME, i);
            fflush(stderr);
            exit(1);
        }
        if (entry.operation == LEDGER_OP_INSERT)
        {
            printf("[%lu] INSERT %d\n", i, entry.val);
        }
        else if (entry.operation == LEDGER_OP_DELETE)
        {
            printf("[%lu] DELETE %d\n", i, entry.val);
        }
    }
    fflush(stdout);
    return(0);
}

