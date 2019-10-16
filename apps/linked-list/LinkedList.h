#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "AccessPointer.h"
#include "DataItem.h"
#include "Link.h"

extern int NUM_OF_EXTRA_LINKS;
extern unsigned long VERSION_STAMP;
extern char AP_WOOF_NAME[255];
extern unsigned long AP_WOOF_SIZE;
extern char DATA_WOOF_NAME[255];
extern unsigned long DATA_WOOF_SIZE;
extern unsigned long LINK_WOOF_SIZE;
extern int LINK_WOOF_NAME_SIZE;
extern int NUM_OF_LINKS_PER_NODE;
extern int CHECKPOINT_MAX_ELEMENTS;

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
 * add child node to parent node
 **/
void add_node(unsigned long version_stamp, AP parent, AP child);

/**
 * populate current link
 **/
void populate_current_link(unsigned long version_stamp, AP node, LINK *current_link);

/**
 * populate terminal node
 **/
void populate_terminal_node(AP *terminal_node);

/**
 * insert into linked list
 **/
void LL_insert(DI di);

/**
 * search for di in version_stamp
 **/
void search(unsigned long version_stamp, DI di, AP *node);

/**
 * delete di
 **/
void LL_delete(DI di);

/**
 * print linked list
 **/
void LL_print(unsigned long version_stamp);

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

#endif
