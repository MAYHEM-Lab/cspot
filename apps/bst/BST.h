#ifndef BST_H
#define BST_H

#include "DataItem.h"

extern int NUM_OF_EXTRA_LINKS;
extern unsigned long VERSION_STAMP;
extern char AP_WOOF_NAME[255];
extern unsigned long AP_WOOF_SIZE;
extern char DATA_WOOF_NAME[255];
extern unsigned long DATA_WOOF_SIZE;

void BST_init(int num_of_extra_links, char *ap_woof_name, unsigned long ap_woof_size, char *data_woof_name, unsigned long data_woof_size);
void BST_insert(DI di);

#endif
