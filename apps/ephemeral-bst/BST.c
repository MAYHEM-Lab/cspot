#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "BST.h"
#include "Helper.h"
#include "AccessPointer.h"
#include "Data.h"
#include "Link.h"

#define LOG_ENABLED 1

unsigned long VERSION_STAMP = 0;
char AP_WOOF_NAME[255];
unsigned long AP_WOOF_SIZE;
char DATA_WOOF_NAME[255];
unsigned long DATA_WOOF_SIZE;
unsigned long LINK_WOOF_SIZE;
int WOOF_NAME_SIZE;
char *WORKLOAD_SUFFIX;
char LOG_FILENAME[255]; //woof access
char SECONDARY_LOG_FILENAME[255]; //steps
char SPACE_LOG_FILENAME[255]; //space
int NUM_STEPS;
FILE *fp;

void BST_init(unsigned long ap_woof_size, unsigned long data_woof_size, unsigned long link_woof_size){
    srand(time(0));
    WOOF_NAME_SIZE = 20;
    strcpy(AP_WOOF_NAME, getRandomWooFName(WOOF_NAME_SIZE));
    AP_WOOF_SIZE = ap_woof_size;
    createWooF(AP_WOOF_NAME, sizeof(AP), ap_woof_size);
    strcpy(DATA_WOOF_NAME, getRandomWooFName(WOOF_NAME_SIZE));
    DATA_WOOF_SIZE = data_woof_size;
    LINK_WOOF_SIZE = link_woof_size;
    createWooF(DATA_WOOF_NAME, sizeof(DATA), data_woof_size);
    VERSION_STAMP = 0;
#if LOG_ENABLED
    strcpy(LOG_FILENAME, "ephemeral-binary-search-tree-woof-access-");
    strcat(LOG_FILENAME, WORKLOAD_SUFFIX);
    strcat(LOG_FILENAME, ".log");
    fp = fopen(LOG_FILENAME, "w");
    fclose(fp);
    fp = NULL;

    strcpy(SECONDARY_LOG_FILENAME, "ephemeral-binary-search-tree-steps-");
    strcat(SECONDARY_LOG_FILENAME, WORKLOAD_SUFFIX);
    strcat(SECONDARY_LOG_FILENAME, ".log");
    fp = fopen(SECONDARY_LOG_FILENAME, "w");
    fclose(fp);
    fp = NULL;
#endif

    strcpy(SPACE_LOG_FILENAME, "ephemeral-binary-search-tree-space-");
    strcat(SPACE_LOG_FILENAME, WORKLOAD_SUFFIX);
    strcat(SPACE_LOG_FILENAME, ".log");
    fp = fopen(SPACE_LOG_FILENAME, "w");
    fclose(fp);
    fp = NULL;
}

void populate_terminal_node(DI di, unsigned long *dw_seq_no){
    AP ap;
    DATA data;
    LINK left_link;
    LINK right_link;
    unsigned long latest_seq;

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(latest_seq == 0){//empty tree
        *dw_seq_no = 0;
        return;
    }

    WooFGet(AP_WOOF_NAME, (void *)&ap, latest_seq);
    if(ap.dw_seq_no == 0){//empty tree
        *dw_seq_no = 0;
        return;
    }

    while(1){
        WooFGet(DATA_WOOF_NAME, (void *)&data, ap.dw_seq_no);
        latest_seq = WooFGetLatestSeqno(data.lw_name);
        WooFGet(data.lw_name, (void *)&right_link, latest_seq);
        if(di.val < data.di.val){
            WooFGet(data.lw_name, (void *)&left_link, latest_seq - 1);
            if(left_link.dw_seq_no == 0){//we found the terminal node
                *dw_seq_no = ap.dw_seq_no;
                return;
            }else{
                ap.dw_seq_no = left_link.dw_seq_no;
            }
        }else{
            WooFGet(data.lw_name, (void *)&right_link, latest_seq);
            if(right_link.dw_seq_no == 0){//we found the terminal node
                *dw_seq_no = ap.dw_seq_no;
                return;
            }else{
                ap.dw_seq_no = right_link.dw_seq_no;
            }
        }
    }
}

char which_child(unsigned long parent_dw, unsigned long child_dw){

    char ret;
    DATA data;
    LINK link;
    unsigned long latest_seq;

    ret = 'N';

    if(parent_dw == 0) return ret;

    WooFGet(DATA_WOOF_NAME, (void *)&data, parent_dw);
    latest_seq = WooFGetLatestSeqno(data.lw_name);

    WooFGet(data.lw_name, (void *)&link, latest_seq - 1);
    if(link.dw_seq_no == child_dw) return 'L';

    WooFGet(data.lw_name, (void *)&link, latest_seq);
    if(link.dw_seq_no == child_dw) return 'R';

    return ret;

}

void search_BST(DI di, unsigned long current_dw, unsigned long *dw_seq_no){

    DATA data;
    LINK left_link;
    LINK right_link;
    unsigned long latest_seq;

    if(current_dw == 0){//null
        *dw_seq_no = 0;
        return;
    }

    WooFGet(DATA_WOOF_NAME, (void *)&data, current_dw);
    if(data.di.val == di.val){//found data
        *dw_seq_no = current_dw;
        return;
    }

    latest_seq = WooFGetLatestSeqno(data.lw_name);

    if(di.val < data.di.val){
        WooFGet(data.lw_name, (void *)&left_link, latest_seq - 1);
        search_BST(di, left_link.dw_seq_no, dw_seq_no);
    }else{
        WooFGet(data.lw_name, (void *)&right_link, latest_seq);
        search_BST(di, right_link.dw_seq_no, dw_seq_no);
    }
    
}

void BST_search(DI di, unsigned long *dw_seq_no){
    AP ap;
    unsigned long latest_seq;

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    WooFGet(AP_WOOF_NAME, (void *)&ap, latest_seq);
    search_BST(di, ap.dw_seq_no, dw_seq_no);
}

void BST_insert(DI di){
    unsigned long ndx;
    unsigned long target_dw_seq_no;
    unsigned long latest_seq;
    DATA data;
    DATA parent_data;
    LINK link;
    LINK left_link;
    LINK right_link;
    AP ap;

    target_dw_seq_no = 0;
    BST_search(di, &target_dw_seq_no);
    if(target_dw_seq_no != 0){//already present
        return;
    }

    VERSION_STAMP += 1;

#if LOG_ENABLED
    NUM_STEPS = 0;
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "INSERT START:%lu\n", VERSION_STAMP);
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif

#if LOG_ENABLED
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "DATA:new data\n");
        fprintf(fp, "LINK:new child\n");
        fprintf(fp, "LINK:new child\n");
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif
    data.di = di;
    strcpy(data.lw_name, getRandomWooFName(WOOF_NAME_SIZE));
    strcpy(data.pw_name, getRandomWooFName(WOOF_NAME_SIZE));
    ndx = insertIntoWooF(DATA_WOOF_NAME, NULL, (void *)&data);
    WooFCreate(data.lw_name, sizeof(LINK), LINK_WOOF_SIZE);
    WooFCreate(data.pw_name, sizeof(LINK), LINK_WOOF_SIZE);
    left_link.dw_seq_no = 0;
    left_link.type = 'L';
    right_link.dw_seq_no = 0;
    right_link.type = 'R';

    insertIntoWooF(data.lw_name, NULL, (void *)&left_link);
    insertIntoWooF(data.lw_name, NULL, (void *)&right_link);

    ap.dw_seq_no = 0;
    populate_terminal_node(di, &ap.dw_seq_no);
    if(ap.dw_seq_no == 0){//empty tree
#if LOG_ENABLED
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "LINK:new parent\n");
        }
        fflush(fp);
        fclose(fp);
        fp = NULL;
#endif
        link.dw_seq_no = ap.dw_seq_no;
        link.type = 'P';
        insertIntoWooF(data.pw_name, NULL, (void *)&link);
        ap.dw_seq_no = ndx;
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
#if LOG_ENABLED
        fp = fopen(SECONDARY_LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "1\n");
            fflush(fp);
        }
        fflush(fp);
        fclose(fp);
        fp = NULL;

        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "INSERT END:%lu\n", VERSION_STAMP);
        }
        fflush(fp);
        fclose(fp);
        fp = NULL;
#endif
        return;
    }

#if LOG_ENABLED
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "DATA:get parent\n");
        fprintf(fp, "LINK:get child\n");
        fprintf(fp, "LINK:get child\n");
        fprintf(fp, "LINK:new child\n");
        fprintf(fp, "LINK:new child\n");
        fprintf(fp, "LINK:new parent\n");
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif
    WooFGet(DATA_WOOF_NAME, (void *)&parent_data, ap.dw_seq_no);
    latest_seq = WooFGetLatestSeqno(parent_data.lw_name);
    WooFGet(parent_data.lw_name, (void *)&left_link, latest_seq - 1);
    WooFGet(parent_data.lw_name, (void *)&right_link, latest_seq);

    if(parent_data.di.val > di.val){
        left_link.dw_seq_no = ndx;
    }else{
        right_link.dw_seq_no = ndx;
    }
    insertIntoWooF(parent_data.lw_name, NULL, (void *)&left_link);
    insertIntoWooF(parent_data.lw_name, NULL, (void *)&right_link);

    link.dw_seq_no = ap.dw_seq_no;
    link.type = 'P';
    insertIntoWooF(data.pw_name, NULL, (void *)&link);

#if LOG_ENABLED
    fp = fopen(SECONDARY_LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "1\n");
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;

    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "INSERT END:%lu\n", VERSION_STAMP);
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif

}

void populate_predecessor(unsigned long target_dw, unsigned long *pred_dw){

    DATA data;
    LINK left_link;
    LINK right_link;
    unsigned long latest_seq;
    unsigned long candidate;

    WooFGet(DATA_WOOF_NAME, (void *)&data, target_dw);
    latest_seq = WooFGetLatestSeqno(data.lw_name);
    WooFGet(data.lw_name, (void *)&left_link, latest_seq - 1);

    if(left_link.dw_seq_no == 0){
        *pred_dw = 0;
        return;
    }

    candidate = left_link.dw_seq_no;
    while(1){
        WooFGet(DATA_WOOF_NAME, (void *)&data, candidate);
        latest_seq = WooFGetLatestSeqno(data.lw_name);
        WooFGet(data.lw_name, (void *)&right_link, latest_seq);
        if(right_link.dw_seq_no == 0){
            *pred_dw = candidate;
            return;
        }
        candidate = right_link.dw_seq_no;
    }

}

void add_node(unsigned long parent_dw, unsigned long child_dw, char type){

    DATA parent_data;
    DATA child_data;
    LINK parent_left;
    LINK parent_right;
    LINK parent;
    unsigned long latest_seq;

#if LOG_ENABLED
    NUM_STEPS += 1;
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "add_node START\n");
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif

    if(parent_dw != 0){
#if LOG_ENABLED
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "DATA:get parent\n");
            fprintf(fp, "LINK:get child\n");
            fprintf(fp, "LINK:get child\n");
            fprintf(fp, "LINK:new child\n");
            fprintf(fp, "LINK:new child\n");
        }
        fflush(fp);
        fclose(fp);
        fp = NULL;
#endif
        WooFGet(DATA_WOOF_NAME, (void *)&parent_data, parent_dw);
        latest_seq = WooFGetLatestSeqno(parent_data.lw_name);
        WooFGet(parent_data.lw_name, (void *)&parent_left, latest_seq - 1);
        WooFGet(parent_data.lw_name, (void *)&parent_right, latest_seq);
        if(type == 'L'){
            parent_left.dw_seq_no = child_dw;
        }else if(type == 'R'){
            parent_right.dw_seq_no = child_dw;
        }
        insertIntoWooF(parent_data.lw_name, NULL, (void *)&parent_left);
        insertIntoWooF(parent_data.lw_name, NULL, (void *)&parent_right);
    }

    if(child_dw != 0){
#if LOG_ENABLED
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "DATA:get child\n");
            fprintf(fp, "LINK:new parent\n");
        }
        fflush(fp);
        fclose(fp);
        fp = NULL;
#endif
        WooFGet(DATA_WOOF_NAME, (void *)&child_data, child_dw);
        parent.dw_seq_no = parent_dw;
        insertIntoWooF(child_data.pw_name, NULL, (void *)&parent);
    }

#if LOG_ENABLED
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "add_node END\n");
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif
}

void BST_delete(DI di){

    AP ap;
    DATA data;
    DATA parent_data;
    DATA pred_data;
    DATA pred_parent_data;
    DATA aux_data;
    LINK left_link;
    LINK right_link;
    LINK aux_left_link;
    LINK aux_right_link;
    LINK parent_link;
    LINK pred_parent_link;
    LINK link;
    unsigned long latest_seq;
    unsigned long target_dw;
    unsigned long pred_dw;

    target_dw = 0;
    BST_search(di, &target_dw);

    if(target_dw == 0){//not present
        return;
    }

    VERSION_STAMP += 1;

#if LOG_ENABLED
    NUM_STEPS = 0;
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "DELETE START:%lu\n", VERSION_STAMP);
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif

    //get target data
    WooFGet(DATA_WOOF_NAME, (void *)&data, target_dw);
    latest_seq = WooFGetLatestSeqno(data.lw_name);
    //get target left and right
    WooFGet(data.lw_name, (void *)&left_link, latest_seq - 1);
    WooFGet(data.lw_name, (void *)&right_link, latest_seq);

    latest_seq = WooFGetLatestSeqno(data.pw_name);
    //get target parent
    WooFGet(data.pw_name, (void *)&parent_link, latest_seq);

    if(left_link.dw_seq_no == 0 && right_link.dw_seq_no == 0){//both children absent
        add_node(parent_link.dw_seq_no, 0, which_child(parent_link.dw_seq_no, target_dw));
        if(parent_link.dw_seq_no == 0){
            ap.dw_seq_no = 0;
            insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
        }
    }else if(right_link.dw_seq_no == 0){//right child absent
        add_node(parent_link.dw_seq_no, left_link.dw_seq_no, which_child(parent_link.dw_seq_no, target_dw));
        if(parent_link.dw_seq_no == 0){
            ap.dw_seq_no = left_link.dw_seq_no;
            insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
        }
    }else if(left_link.dw_seq_no == 0){//left child absent
        add_node(parent_link.dw_seq_no, right_link.dw_seq_no, which_child(parent_link.dw_seq_no, target_dw));
        if(parent_link.dw_seq_no == 0){
            ap.dw_seq_no = right_link.dw_seq_no;
            insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
        }
    }else{//both children present
        populate_predecessor(target_dw, &pred_dw);
        WooFGet(DATA_WOOF_NAME, (void *)&pred_data, pred_dw);
        if(pred_dw == left_link.dw_seq_no){//predecessor is immediate left child
            add_node(pred_dw, right_link.dw_seq_no, 'R');
            add_node(parent_link.dw_seq_no, pred_dw, which_child(parent_link.dw_seq_no, target_dw));
            if(parent_link.dw_seq_no == 0){
                ap.dw_seq_no = pred_dw;
                insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
            }
        }else{//predecessor not immediate left child
            latest_seq = WooFGetLatestSeqno(pred_data.pw_name);
            //get pred parent link
            WooFGet(pred_data.pw_name, (void *)&pred_parent_link, latest_seq);
            //get pred left link
            latest_seq = WooFGetLatestSeqno(pred_data.lw_name);
            WooFGet(pred_data.lw_name, (void *)&aux_left_link, latest_seq - 1);
            //step 1: add pred parent and pred left
            add_node(pred_parent_link.dw_seq_no, aux_left_link.dw_seq_no, 'R');
            //step 2: add pred and left
            add_node(pred_dw, left_link.dw_seq_no, 'L');
            //step 3: add pred and right
            add_node(pred_dw, right_link.dw_seq_no, 'R');
            //step 4: add parent and pred
            add_node(parent_link.dw_seq_no, pred_dw, which_child(parent_link.dw_seq_no, target_dw));
            if(parent_link.dw_seq_no == 0){
                ap.dw_seq_no = pred_dw;
                insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
            }
        }

    }

#if LOG_ENABLED
    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "LINK:new parent\n");
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif
    //make target parent null
    link.dw_seq_no = 0;
    link.type = 'P';
    insertIntoWooF(data.pw_name, NULL, (void *)&link);
#if LOG_ENABLED
    fp = fopen(SECONDARY_LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "%d\n", NUM_STEPS);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;

    fp = fopen(LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "DELETE END:%lu\n", VERSION_STAMP);
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
#endif

}

void preorder_BST(unsigned long dw_seq_no){

    DATA data;
    LINK left_link;
    LINK right_link;
    unsigned long latest_seq;

    if(dw_seq_no == 0){//null
        return;
    }

    WooFGet(DATA_WOOF_NAME, (void *)&data, dw_seq_no);
    fprintf(stdout, "%d ", data.di);
    fflush(stdout);

    latest_seq = WooFGetLatestSeqno(data.lw_name);
    WooFGet(data.lw_name, (void *)&left_link, latest_seq - 1);
    WooFGet(data.lw_name, (void *)&right_link, latest_seq);

    preorder_BST(left_link.dw_seq_no);
    preorder_BST(right_link.dw_seq_no);

}

void BST_preorder(){
    AP ap;
    unsigned long latest_seq;

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    fprintf(stdout, "%lu: ", VERSION_STAMP);
    if(latest_seq > 0){
        WooFGet(AP_WOOF_NAME, (void *)&ap, latest_seq);
        preorder_BST(ap.dw_seq_no);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

void dump_link_woof(char *name){
    unsigned long i;
    LINK link;
    fprintf(stdout, "dumping link woof: %s\n", name);
    for(i = 1; i <= WooFGetLatestSeqno(name); ++i){
        WooFGet(name, (void *)&link, i);
        fprintf(stdout, "%lu: %lu %lu %c %lu\n", i, link.dw_seq_no, link.type);
        fflush(stdout);
    }
}

void dump_data_woof(){
    unsigned long i;
    DATA data;
    for(i = 1; i <= WooFGetLatestSeqno(DATA_WOOF_NAME); ++i){
        WooFGet(DATA_WOOF_NAME, (void *)&data, i);
        fprintf(stdout, "%lu: %c %s %s\n", i, data.di.val, data.lw_name, data.pw_name);
        fflush(stdout);
    }
}

void dump_ap_woof(){
    unsigned long i;
    AP ap;
    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        WooFGet(AP_WOOF_NAME, (void *)&ap, i);
        fprintf(stdout, "%lu: %lu\n", i, ap.dw_seq_no);
        fflush(stdout);
    }
}

void BST_debug(){

    dump_data_woof();

}

void log_size(int num_ops_input){
    
    DATA data;
    unsigned long latest_seq_data_woof;
    unsigned long latest_seq;
    unsigned long i;
    size_t total_size = 0;

    latest_seq_data_woof = WooFGetLatestSeqno(DATA_WOOF_NAME);
    total_size += (latest_seq_data_woof * sizeof(DATA));
    for(i = 1; i <= latest_seq_data_woof; ++i){
        WooFGet(DATA_WOOF_NAME, (void *)&data, i);
        latest_seq = WooFGetLatestSeqno(data.lw_name);
        total_size += (latest_seq * sizeof(LINK));
        latest_seq = WooFGetLatestSeqno(data.pw_name);
        total_size += (latest_seq * sizeof(LINK));
        break;
    }

    fp = fopen(SPACE_LOG_FILENAME, "a");
    if(fp != NULL){
        fprintf(fp, "%d,%zu\n", num_ops_input, total_size);
        fflush(fp);
        fclose(fp);
        fp = NULL;
    }

}
