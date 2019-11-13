#ifndef DATA_ITEM_H
#define DATA_ITEM_H

#include <stdint.h>

#define S3_KEY_SIZE 256

struct DataItem{
    char key[S3_KEY_SIZE];
    int64_t logId;
    int64_t recordIdx;
    int64_t size;
};

typedef struct DataItem DI;

char *DI_get_str(DI di);

#endif
