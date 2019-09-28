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

void populate_current_left_right(unsigned long version_stamp, unsigned long dw_seq_no, unsigned long lw_seq_no, unsigned long *left_dw_seq_no, unsigned long *left_lw_seq_no, unsigned long *left_vs, unsigned long *right_dw_seq_no, unsigned long *right_lw_seq_no, unsigned long *right_vs){
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
                    *left_vs = link.version_stamp;
                    max_left = link.version_stamp;
                }
            }else if(link.type == 'R'){
                if(link.version_stamp >= max_right && link.version_stamp <= version_stamp){
                    *right_dw_seq_no = link.dw_seq_no;
                    *right_lw_seq_no = link.lw_seq_no;
                    *right_vs = link.version_stamp;
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
    unsigned long left_vs;
    unsigned long right_vs;

    if(WooFGetLatestSeqno(AP_WOOF_NAME) < version_stamp){//empty tree
        *dw_seq_no = 0;
        *lw_seq_no = 0;
        return;
    }

    WooFGet(AP_WOOF_NAME, (void *)&ap, version_stamp);
    while(1){
        populate_current_left_right(version_stamp, ap.dw_seq_no, ap.lw_seq_no, &left_dw_seq_no, &left_lw_seq_no, &left_vs, &right_dw_seq_no, &right_lw_seq_no, &right_vs);
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

char which_child(unsigned long parent_dw, unsigned long parent_lw, unsigned long child_dw, unsigned long child_lw){

    char ret;
    ret = 'N';
    DATA data;
    LINK link;
    unsigned long i;
    unsigned long end;
    unsigned long max;

    if(parent_dw == 0) return ret;

    end = parent_lw + NUM_OF_ENTRIES_PER_NODE - 1;
    WooFGet(DATA_WOOF_NAME, (void *)&data, parent_dw);
    max = WooFGetLatestSeqno(data.lw_name);
    for(i = parent_lw; i <= end; ++i){
        if(i <= max){
            WooFGet(data.lw_name, (void *)&link, i);
            if(link.dw_seq_no == child_dw && link.lw_seq_no == child_lw){
                return link.type;
            }
        }
    }

    return ret;
}

void add_node(unsigned long working_vs, char type, unsigned long child_dw, unsigned long child_lw, unsigned long parent_dw, unsigned long parent_lw){

    AP ap;
    DATA data;
    LINK link, link_1;
    unsigned long max_seq;

    unsigned long left_dw_seq_no;
    unsigned long left_lw_seq_no;
    unsigned long right_dw_seq_no;
    unsigned long right_lw_seq_no;
    unsigned long left_vs;
    unsigned long right_vs;

    if(parent_dw == 0){//null
        fprintf(stdout, "{%lu} !!!PARENT NULL!!!\n", working_vs);
        fflush(stdout);
        ap.dw_seq_no = child_dw;
        ap.lw_seq_no = child_lw;
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
        return;
    }

    //can fit in
    WooFGet(DATA_WOOF_NAME, (void *)&data, parent_dw);
    max_seq = WooFGetLatestSeqno(data.lw_name);
    if((max_seq - parent_lw + 1) < NUM_OF_ENTRIES_PER_NODE){//no need to copy
        // add child
        link.dw_seq_no = child_dw;
        link.lw_seq_no = child_lw;
        link.type = type;
        link.version_stamp = working_vs;
        insertIntoWooF(data.lw_name, NULL, (void *)&link);
        // add parent
        WooFGet(DATA_WOOF_NAME, (void *)&data, child_dw);
        link.dw_seq_no = parent_dw;
        link.lw_seq_no = parent_lw;
        link.type = 'P';
        link.version_stamp = working_vs;
        insertIntoWooF(data.pw_name, NULL, (void *)&link);
        return;
    }

    populate_current_left_right(VERSION_STAMP, parent_dw, parent_lw, &left_dw_seq_no, &left_lw_seq_no, &left_vs, &right_dw_seq_no, &right_lw_seq_no, &right_vs);
    if(type == 'L'){//copy right
        link.dw_seq_no = right_dw_seq_no;
        link.lw_seq_no = right_lw_seq_no;
        link.version_stamp = right_vs;
        link.type = 'R';
        WooFGet(DATA_WOOF_NAME, (void *)&data, parent_dw);
        insertIntoWooF(data.lw_name, NULL, (void *)&link);
    }else{//copy left
        link.dw_seq_no = left_dw_seq_no;
        link.lw_seq_no = left_lw_seq_no;
        link.version_stamp = left_vs;
        link.type = 'L';
        WooFGet(DATA_WOOF_NAME, (void *)&data, parent_dw);
        insertIntoWooF(data.lw_name, NULL, (void *)&link);
    }
    //add new link to parent
    link.dw_seq_no = child_dw;
    link.lw_seq_no = child_lw;
    link.version_stamp = working_vs;
    link.type = type;
    insertIntoWooF(data.lw_name, NULL, (void *)&link);
    //set parent
    link.dw_seq_no = parent_dw;
    link.lw_seq_no = max_seq + 1;
    link.type = 'P';
    link.version_stamp = working_vs;
    WooFGet(DATA_WOOF_NAME, (void *)&data, child_dw);
    insertIntoWooF(data.pw_name, NULL, (void *)&link);
    //also if parent_dw happens to be head, update AP
    WooFGet(AP_WOOF_NAME, (void *)&ap, WooFGetLatestSeqno(AP_WOOF_NAME));
    if(ap.dw_seq_no == link.dw_seq_no){
        ap.lw_seq_no = link.lw_seq_no;
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
    }

    //WooFGet(DATA_WOOF_NAME, (void *)&data, parent_dw);
    WooFGet(DATA_WOOF_NAME, (void *)&data, child_dw);
    child_dw = parent_dw;
    child_lw = parent_lw;
    WooFGet(data.pw_name, (void *)&link, WooFGetLatestSeqno(data.pw_name));
    parent_dw = link.dw_seq_no;
    parent_lw = link.lw_seq_no;
    type = which_child(parent_dw, parent_lw, child_dw, child_lw);
    add_node(working_vs, type, child_dw, child_lw, parent_dw, parent_lw);
}

void BST_insert(DI di){
    unsigned long working_vs;
    unsigned long ndx;
    DATA data;
    DATA parent_data;
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
        ndx = insertIntoWooF(DATA_WOOF_NAME, NULL, (void *)&data);
        WooFCreate(data.lw_name, sizeof(LINK), 100);
        WooFCreate(data.pw_name, sizeof(LINK), 100);
        link.dw_seq_no = 0;
        link.lw_seq_no = 0;
        link.version_stamp = data.version_stamp;
        link.type = 'L';
        WooFPut(data.lw_name, NULL, (void *)&link);
        link.type = 'R';
        WooFPut(data.lw_name, NULL, (void *)&link);
        link.type = 'P';
        WooFPut(data.pw_name, NULL, (void *)&link);
        /* insert into ap woof */
        ap.dw_seq_no = ndx;
        ap.lw_seq_no = 1;
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
        VERSION_STAMP = working_vs;
        return;
    }

    data.di = di;
    strcpy(data.lw_name, getRandomWooFName(WOOF_NAME_SIZE));
    strcpy(data.pw_name, getRandomWooFName(WOOF_NAME_SIZE));
    data.version_stamp = working_vs;
    ndx = insertIntoWooF(DATA_WOOF_NAME, NULL, (void *)&data);
    WooFCreate(data.lw_name, sizeof(LINK), 100);
    WooFCreate(data.pw_name, sizeof(LINK), 100);
    link.dw_seq_no = 0;
    link.lw_seq_no = 0;
    link.version_stamp = data.version_stamp;
    link.type = 'L';
    WooFPut(data.lw_name, NULL, (void *)&link);
    link.type = 'R';
    WooFPut(data.lw_name, NULL, (void *)&link);
    link.type = 'P';
    WooFPut(data.pw_name, NULL, (void *)&link);

    WooFGet(DATA_WOOF_NAME, (void *)&parent_data, ap.dw_seq_no);
    add_node(working_vs, (data.di.val < parent_data.di.val) ? 'L' : 'R', ndx, 1, ap.dw_seq_no, ap.lw_seq_no);

    ndx = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(ndx < working_vs){
        WooFGet(AP_WOOF_NAME, (void *)&ap, ndx);
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
    }
    VERSION_STAMP = working_vs;

}

void preorder_BST(unsigned long version_stamp, unsigned long dw_seq_no, unsigned long lw_seq_no){

    DATA data;
    unsigned long left_dw_seq_no;
    unsigned long left_lw_seq_no;
    unsigned long right_dw_seq_no;
    unsigned long right_lw_seq_no;
    unsigned long left_vs;
    unsigned long right_vs;

    if(dw_seq_no == 0){//null
        return;
    }

    populate_current_left_right(version_stamp, dw_seq_no, lw_seq_no, &left_dw_seq_no, &left_lw_seq_no, &left_vs, &right_dw_seq_no, &right_lw_seq_no, &right_vs);

    WooFGet(DATA_WOOF_NAME, (void *)&data, dw_seq_no);
    fprintf(stdout, "%c ", data.di);
    fflush(stdout);

    preorder_BST(version_stamp, left_dw_seq_no, left_lw_seq_no);
    preorder_BST(version_stamp, right_dw_seq_no, right_lw_seq_no);

}

void BST_preorder(unsigned long version_stamp){
    AP ap;
    WooFGet(AP_WOOF_NAME, (void *)&ap, version_stamp);
    preorder_BST(version_stamp, ap.dw_seq_no, ap.lw_seq_no);
    fprintf(stdout, "\n");
    fflush(stdout);
}

void BST_debug(){
    unsigned long i;
    LINK link;
    AP ap;

    char woof_name[] = "bynecdyggx";
    //E - nwlrbbmqbh cdarzowkky
    //C - hiddqscdxr jmowfrxsjy
    //M - bldbefsarc bynecdyggx
    for(i = 1; i <= WooFGetLatestSeqno(woof_name); ++i){
        WooFGet(woof_name, (void *)&link, i);
        fprintf(stdout, "%lu %lu %c %lu\n", link.dw_seq_no, link.lw_seq_no, link.type, link.version_stamp);
        fflush(stdout);
    }

    //AP
    for(i = 1; i <= WooFGetLatestSeqno("AP"); ++i){
        WooFGet("AP", (void *)&ap, i);
        fprintf(stdout, "%lu: %lu %lu\n", i, ap.dw_seq_no, ap.lw_seq_no);
        fflush(stdout);
    }

}
