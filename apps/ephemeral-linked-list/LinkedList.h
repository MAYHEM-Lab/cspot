#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "AccessPointer.h"
#include "DataItem.h"
#include "Link.h"

extern char AP_WOOF_NAME[255];
extern unsigned long AP_WOOF_SIZE;
extern char DATA_WOOF_NAME[255];
extern unsigned long DATA_WOOF_SIZE;
extern unsigned long LINK_WOOF_SIZE;
extern int LINK_WOOF_NAME_SIZE;
extern unsigned long VERSION_STAMP;
extern char *WORKLOAD_SUFFIX;
extern unsigned long EXTRA_TIME;

/**
 * initializes state variables
 **/
void LL_init(
        int num_of_extra_links,
        unsigned long ap_woof_size,
        unsigned long data_woof_size,
        unsigned long link_woof_size
        );

/**
 * populate terminal node
 **/
void populate_terminal_node(AP *terminal_node);

/**
 * populate terminal node
 **/
void populate_terminal_node_access(AP *terminal_node);

/**
 * insert into linked list
 **/
void LL_insert(DI di);

/**
 * search for di
 **/
void search(DI di, AP *node);

/**
 * delete di
 **/
void LL_delete(DI di);

/**
 * print linked list
 **/
void LL_print();

/**
 * helper debugger
 * displays LINK woof
 **/
void debug_LINK(char *name);

/**
 * helper debugger
 * displays DATA woof
 **/
void debug_DATA();

/**
 * helper debugger
 * displays AP woof
 **/
void debug_AP();

/**
 * debug
 **/
void LL_debug();

void log_size(int num_ops_input, FILE *fp_s);

#endif
