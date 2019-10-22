#include <stdio.h>

#include "ledger.h"
#include "woofc.h"

void ledger_init(int data_structure)
{
    int err;
    
    ledger_data_structure = data_structure;
    err = WooFCreate(LEDGER_WOOF_NAME, sizeof(LEDGER_ENTRY), DEFAULT_WOOF_LOG_SIZE);
    if (err < 0)
    {
        fprintf(stderr, "Couldn't create %s\n", LEDGER_WOOF_NAME);
        fflush(stderr);
        exit(1);
    }
}

void ledger_insert(int val)
{
    unsigned long seq;
    LEDGER_ENTRY entry;
    entry.operation = LEDGER_OP_INSERT;
    entry.val = val;
    seq = WooFPut(LEDGER_WOOF_NAME, NULL, &entry);
    if (WooFInvalid(seq))
    {
        fprintf(stderr, "Couldn't log to ledger\n");
        fflush(stderr);
        exit(1);
    }
}

void ledger_delete(int val)
{
    unsigned long seq;
    LEDGER_ENTRY entry;
    entry.operation = LEDGER_OP_DELETE;
    entry.val = val;
    seq = WooFPut(LEDGER_WOOF_NAME, NULL, &entry);
    if (WooFInvalid(seq))
    {
        fprintf(stderr, "Couldn't log to ledger\n");
        fflush(stderr);
        exit(1);
    }
}
