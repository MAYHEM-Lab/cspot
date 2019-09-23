#ifndef HELPER_H
#define HELPER_H

int createWooF(char *woof_name, unsigned long element_size, unsigned long history_size);
unsigned long insertIntoWooF(char *woof_name, char *handler_name, void *element);

#endif
