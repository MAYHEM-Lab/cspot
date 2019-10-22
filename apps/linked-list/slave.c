#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "ledger.h"
#include "LinkedList.h"

#define ARGS "w:"
char *Usage = "slave -w master_woof\n";

char master_woof[256];

int main(int argc, char **argv)
{
    int c;
    char master_ledger[256];
    unsigned long master_seqno;
    unsigned long local_seqno;
    unsigned long i;
    int err;
    LEDGER_ENTRY entry;
    DI di;

    while ((c = getopt(argc, argv, ARGS)) != EOF)
    {
        switch (c)
        {
        case 'w':
            strncpy(master_woof, optarg, sizeof(master_woof));
            break;
        default:
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
    }

    WooFInit();
    LL_init(3, 10000, 10000, 10000);
    ledger_init(LEDGER_DS_LINKEDLIST);
    sprintf(master_ledger, "%s/%s", master_woof, LEDGER_WOOF_NAME);

    while (1)
    {
        master_seqno = WooFGetLatestSeqno(master_ledger);
        if (WooFInvalid(master_seqno))
        {
            fprintf(stderr, "Couldn't get the latest seqno of %s\n", master_ledger);
            fflush(stderr);
            exit(1);
        }
        printf("%lu entries in master ledger\n", master_seqno);

        local_seqno = WooFGetLatestSeqno(LEDGER_WOOF_NAME);
        if (WooFInvalid(local_seqno))
        {
            fprintf(stderr, "Couldn't get the latest seqno of %s\n", LEDGER_WOOF_NAME);
            fflush(stderr);
            exit(1);
        }
        printf("%lu entries in local ledger\n", local_seqno);

        for (i = local_seqno + 1; i <= master_seqno; i++)
        {
            err = WooFGet(master_ledger, &entry, i);
            if (err < 0)
            {
                fprintf(stderr, "Couldn't read %s[%lu]\n", master_ledger, i);
                fflush(stderr);
                exit(1);
            }
            if (entry.operation == LEDGER_OP_INSERT)
            {
                printf("[%lu] INSERT %d\n", i, entry.val);
                di.val = entry.val;
                LL_insert(di);
            }
            else if (entry.operation == LEDGER_OP_DELETE)
            {
                printf("[%lu] DELETE %d\n", i, entry.val);
                di.val = entry.val;
                LL_delete(di);
            }
        }
        sleep(5);
    }

    return (0);
}
