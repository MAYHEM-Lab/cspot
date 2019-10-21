#ifndef DATA_ITEM_H
#define DATA_ITEM_H

struct DataItem{
    char val;
};

typedef struct DataItem DI;

char *DI_get_str(DI di);

#endif
