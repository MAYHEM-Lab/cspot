#include <stdio.h>
#include <string.h>

#include "BST.h"
#include "Helper.h"
#include "AccessPointer.h"
#include "Data.h"
#include "Link.h"

int NUM_OF_EXTRA_LINKS = 1;
unsigned long VERSION_STAMP = 0;
char AP_WOOF_NAME[255];
unsigned long AP_WOOF_SIZE;
char DATA_WOOF_NAME[255];
unsigned long DATA_WOOF_SIZE;
int WOOF_NAME_SIZE;
int NUM_OF_ENTRIES_PER_NODE;

void BST_init(int num_of_extra_links, char *ap_woof_name, unsigned long ap_woof_size, char *data_woof_name, unsigned long data_woof_size){
    WOOF_NAME_SIZE = 10;
    NUM_OF_EXTRA_LINKS = num_of_extra_links;
    NUM_OF_ENTRIES_PER_NODE = NUM_OF_EXTRA_LINKS + 2;
    strcpy(AP_WOOF_NAME, ap_woof_name);
    AP_WOOF_SIZE = ap_woof_size;
    createWooF(ap_woof_name, sizeof(AP), ap_woof_size);
    strcpy(DATA_WOOF_NAME, data_woof_name);
    DATA_WOOF_SIZE = data_woof_size;
    createWooF(data_woof_name, sizeof(DATA), data_woof_size);
}

void populate_current_left_right(unsigned long version_stamp, unsigned long dw_seq_no, unsigned long lw_seq_no, unsigned long *left_dw_seq_no, unsigned long *left_lw_seq_no, unsigned long *right_dw_seq_no, unsigned long *right_lw_seq_no){
    DATA data;
    LINK link;
    int i;
    unsigned long max_left;
    unsigned long max_right;

    max_left = 0;
    max_right = 0;

    WooFGet(DATA_WOOF_NAME, (void *)&data, dw_seq_no);
    
    for(i = 0; i < NUM_OF_ENTRIES_PER_NODE; ++i){
        if(WooFGetLatestSeqno(data.lw_name) >= (lw_seq_no + i)){
            WooFGet(data.lw_name, (void *)&link, lw_seq_no + i);
            if(link.type == 'L'){
                if(link.version_stamp >= max_left && link.version_stamp <= version_stamp){
                    *left_dw_seq_no = link.dw_seq_no;
                    *left_lw_seq_no = link.lw_seq_no;
                    max_left = link.version_stamp;
                }
            }else if(link.type == 'R'){
                if(link.version_stamp >= max_right && link.version_stamp <= version_stamp){
                    *right_dw_seq_no = link.dw_seq_no;
                    *right_lw_seq_no = link.lw_seq_no;
                    max_right = link.version_stamp;
                }
            }
        }
    }
}

void populate_terminal_node(unsigned long version_stamp, DI di, unsigned long *dw_seq_no, unsigned long *lw_seq_no){
    AP ap;
    DATA data;
    unsigned long left_dw_seq_no;
    unsigned long left_lw_seq_no;
    unsigned long right_dw_seq_no;
    unsigned long right_lw_seq_no;

    if(WooFGetLatestSeqno(AP_WOOF_NAME) < version_stamp){//empty tree
        *dw_seq_no = 0;
        *lw_seq_no = 0;
        return;
    }

    WooFGet(AP_WOOF_NAME, (void *)&ap, version_stamp);
    while(1){
        populate_current_left_right(version_stamp, ap.dw_seq_no, ap.lw_seq_no, &left_dw_seq_no, &left_lw_seq_no, &right_dw_seq_no, &right_lw_seq_no);
        WooFGet(DATA_WOOF_NAME, (void *)&data, ap.dw_seq_no); 
        if(di.val < data.di.val){
            if(left_dw_seq_no == 0){//we found the terminal node
                *dw_seq_no = ap.dw_seq_no;
                *lw_seq_no = ap.lw_seq_no;
                return;
            }else{
                ap.dw_seq_no = left_dw_seq_no;
                ap.lw_seq_no = left_lw_seq_no;
            }
        }else{
            if(right_dw_seq_no == 0){//we found the terminal node
                *dw_seq_no = ap.dw_seq_no;
                *lw_seq_no = ap.lw_seq_no;
                return;
            }else{
                ap.dw_seq_no = right_dw_seq_no;
                ap.lw_seq_no = right_lw_seq_no;
            }
        }
    }
}

void BST_insert(DI di){
    unsigned long working_vs;
    unsigned long ndx;
    DATA data;
    LINK link;
    AP ap;

    working_vs = VERSION_STAMP + 1;
    ap.dw_seq_no = 0;
    ap.lw_seq_no = 0;
    if(WooFGetLatestSeqno(AP_WOOF_NAME) > 0){
        WooFGet(AP_WOOF_NAME, (void *)&ap, VERSION_STAMP);
        populate_terminal_node(VERSION_STAMP, di, &ap.dw_seq_no, &ap.lw_seq_no);
    }

    if(ap.dw_seq_no == 0){//empty tree
        /* insert into data woof */
        data.di = di;
        strcpy(data.lw_name, getRandomWooFName(WOOF_NAME_SIZE));
        strcpy(data.pw_name, getRandomWooFName(WOOF_NAME_SIZE));
        data.version_stamp = working_vs;
        ndx = insertIntoWooF(DATA_WOOF_NAME, "DataHandler", (void *)&data);
        /* insert into ap woof */
        ap.dw_seq_no = ndx;
        ap.lw_seq_no = 1;
        insertIntoWooF(AP_WOOF_NAME, "AccessPointerHandler", (void *)&ap);
        VERSION_STAMP = working_vs;
        return;
    }

    /* insert into data woof -- change this portion to add node */
    data.di = di;
    strcpy(data.lw_name, getRandomWooFName(WOOF_NAME_SIZE));
    strcpy(data.pw_name, getRandomWooFName(WOOF_NAME_SIZE));
    data.version_stamp = working_vs;
    ndx = insertIntoWooF(DATA_WOOF_NAME, "DataHandler", (void *)&data);
    link.dw_seq_no = ap.dw_seq_no;
    link.lw_seq_no = ap.lw_seq_no;
    link.type = 'P';
    link.version_stamp = working_vs;
    insertIntoWooF(data.pw_name, "LinkHandler", (void *)&link);
    WooFGet(DATA_WOOF_NAME, (void *)&data, link.dw_seq_no);
    link.dw_seq_no = ndx;
    link.lw_seq_no = 1;
    link.version_stamp = working_vs;
    link.type = (di.val < data.di.val) ? 'L' : 'R';
    insertIntoWooF(data.lw_name, "LinkHandler", (void *)&link);

    ndx = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(ndx < working_vs){
        WooFGet(AP_WOOF_NAME, (void *)&ap, ndx);
        insertIntoWooF(AP_WOOF_NAME, "AccessPointerHandler", (void *)&ap);
    }
    VERSION_STAMP = working_vs;

}

void BST_preorder(unsigned long version_stamp){
    AP ap;
    DATA data;
    WooFGet(AP_WOOF_NAME, (void *)&ap, version_stamp);
    WooFGet(DATA_WOOF_NAME, (void *)&data, ap.dw_seq_no);
    fprintf(stdout, "PREORDER %c\n", data.di);
    fflush(stdout);
}
