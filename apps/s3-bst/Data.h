#ifndef DATA_H
#define DATA_H

#include "DataItem.h"

struct Data{

    DI di;
    char lw_name[255];
    char pw_name[255];
    unsigned long version_stamp;

};

typedef struct Data DATA;

void DATA_display(DATA data);

#endif
