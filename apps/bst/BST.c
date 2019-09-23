#include <string.h>

#include "BST.h"
#include "Helper.h"
#include "AccessPointer.h"
#include "Data.h"

int NUM_OF_EXTRA_LINKS = 1;
unsigned long VERSION_STAMP = 0;
char AP_WOOF_NAME[255];
unsigned long AP_WOOF_SIZE;
char DATA_WOOF_NAME[255];
unsigned long DATA_WOOF_SIZE;

void BST_init(int num_of_extra_links, char *ap_woof_name, unsigned long ap_woof_size, char *data_woof_name, unsigned long data_woof_size){
    NUM_OF_EXTRA_LINKS = num_of_extra_links;
    strcpy(AP_WOOF_NAME, ap_woof_name);
    AP_WOOF_SIZE = ap_woof_size;
    createWooF(ap_woof_name, sizeof(AP), ap_woof_size);
    strcpy(DATA_WOOF_NAME, data_woof_name);
    DATA_WOOF_SIZE = data_woof_size;
    createWooF(data_woof_name, sizeof(DATA), data_woof_size);
}

void BST_insert(DI di){
    /* insert into data woof */
    insertIntoWooF(DATA_WOOF_NAME, "DataHandler", (void *)&di);
}
