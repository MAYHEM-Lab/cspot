#ifndef BST_H
#define BST_H

#include "DataItem.h"

extern unsigned long VERSION_STAMP;
extern char AP_WOOF_NAME[255];
extern unsigned long AP_WOOF_SIZE;
extern char DATA_WOOF_NAME[255];
extern unsigned long DATA_WOOF_SIZE;
extern char *WORKLOAD_SUFFIX;
extern unsigned long EXTRA_TIME;

/**
 * initializes state variables
 **/
void BST_init(unsigned long ap_woof_size, unsigned long data_woof_size, unsigned long link_woof_size);

/**
 * populates the node at the end of which new node is attached
 **/
void populate_terminal_node(DI di, unsigned long *dw_seq_no);

/**
 * returns type of child
 **/
char which_child(unsigned long parent_dw, unsigned long child_dw);

/**
 * helper function
 * searches for data
 *
 * di: item to be searched
 * current_dw: dw seq no of current node
 * dw_seq_no: out parameter, seq no of di in data woof
 **/
void search_BST(DI di, unsigned long current_dw, unsigned long *dw_seq_no);

/**
 * searches for di in bst
 **/
void BST_search(DI di, unsigned long *dw_seq_no);

/**
 * inserts data to the bst
 **/
void BST_insert(DI di);

/**
 * helper function
 * populates predecessor node of the given node
 *
 * target_dw: dw seq no of the target node
 * pred_dw: out parameter, dw seq no of the predecessor
 **/
void populate_predecessor(unsigned long target_dw, unsigned long *pred_dw);

void add_node(unsigned long parent_dw, unsigned long child_dw, char type);

/**
 * deletes data from the bst
 **/
void BST_delete(DI di);

/**
 * helper function
 * preorder traversal of BST
 **/
void preorder_BST(unsigned long dw_seq_no);

/**
 * preorder traversal of bst
 **/
void BST_preorder();

void dump_link_woof(char *name);
void dump_data_woof();
void dump_ap_woof();
void BST_debug();

void log_size(int num_ops_input, FILE *fp_s);

#endif
