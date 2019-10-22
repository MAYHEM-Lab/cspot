#ifndef LEDGER_H
#define LEDGER_H

#define LEDGER_WOOF_NAME "ledger"
#define LEDGER_DS_LINKEDLIST 0
#define LEDGER_OP_INSERT 0
#define LEDGER_OP_DELETE 1

int ledger_data_structure;

struct ledger_entry
{
    int operation;
    int val;
};

typedef struct ledger_entry LEDGER_ENTRY;

void ledger_init(int data_structure);
void ledger_insert(int val);
void ledger_delete(int val);

#endif