#ifndef BST_H
#define BST_H

#include "DataItem.h"

extern int NUM_OF_EXTRA_LINKS;
extern unsigned long VERSION_STAMP;
extern char AP_WOOF_NAME[255];
extern unsigned long AP_WOOF_SIZE;
extern char DATA_WOOF_NAME[255];
extern unsigned long DATA_WOOF_SIZE;

/**
 * initializes state variables
 **/
void BST_init(int num_of_extra_links, char *ap_woof_name, unsigned long ap_woof_size, char *data_woof_name, unsigned long data_woof_size);

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

#endif
