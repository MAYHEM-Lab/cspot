#ifndef BST_H
#define BST_H

#include "DataItem.h"

extern int NUM_OF_EXTRA_LINKS;
extern unsigned long VERSION_STAMP;
extern char AP_WOOF_NAME[255];
extern unsigned long AP_WOOF_SIZE;
extern char DATA_WOOF_NAME[255];
extern unsigned long DATA_WOOF_SIZE;
extern int DELETE_OPERATION;

/**
 * initializes state variables
 **/
void BST_init(int num_of_extra_links, char *ap_woof_name, unsigned long ap_woof_size, char *data_woof_name, unsigned long data_woof_size, unsigned long link_woof_size);

/**
 * populates left and right nodes for the given node and version stamp
 **/
void populate_current_left_right(unsigned long version_stamp, unsigned long dw_seq_no, unsigned long lw_seq_no, unsigned long *left_dw_seq_no, unsigned long *left_lw_seq_no, unsigned long *left_vs, unsigned long *right_dw_seq_no, unsigned long *right_lw_seq_no, unsigned long *right_vs);

/**
 * populates the node at the end of which new node is attached
 **/
void populate_terminal_node(unsigned long version_stamp, DI di, unsigned long *dw_seq_no, unsigned long *lw_seq_no);

/**
 * returns type of child
 **/
char which_child(unsigned long parent_dw, unsigned long parent_lw, unsigned long child_dw, unsigned long child_lw);

/**
 * adds a new node to the terminal node
 **/
void add_node(unsigned long working_vs, char type, unsigned long child_dw, unsigned long child_lw, unsigned long parent_dw, unsigned long parent_lw);

/**
 * inserts data to the bst
 **/
void BST_insert(DI di);

/**
 * helper function
 * searches for data
 *
 * di: item to be searched
 * version_stamp: version of bst in which di should be searched
 * current_dw: dw seq no of current node
 * current_lw: lw seq no of current node
 * dw_seq_no: out parameter, seq no of di in data woof
 * lw_seq_no: out parameter, seq no of di in link woof
 **/
void search_BST(DI di, unsigned long version_stamp, unsigned long current_dw, unsigned long current_lw, unsigned long *dw_seq_no, unsigned long *lw_seq_no);

/**
 * searches for di in bst
 **/
void BST_search(DI di, unsigned long version_stamp, unsigned long *dw_seq_no, unsigned long *lw_seq_no);

/**
 * searches for di in latest version of the bst
 **/
unsigned long BST_search_latest(DI di);

/**
 * helper function
 * populates predecessor node of the given node
 *
 * target_dw: dw seq no of the target node
 * target_lw: lw seq no of the target node
 * pred_dw: out parameter, dw seq no of the predecessor
 * pred_lw: out parameter, lw seq no of the predecessor
 **/
void populate_predecessor(unsigned long target_dw, unsigned long target_lw, unsigned long *pred_dw, unsigned long *pred_lw);

/**
 * deletes data from the bst
 **/
void BST_delete(DI di);

/**
 * helper function
 * preorder traversal of BST
 **/
void preorder_BST(unsigned long version_stamp, unsigned long dw_seq_no, unsigned long lw_seq_no);

/**
 * preorder traversal of bst
 **/
void BST_preorder(unsigned long version_stamp);

void dump_link_woof(char *name);
void dump_data_woof();
void dump_ap_woof();
void BST_debug();

void log_size(int num_ops_input);

#endif
