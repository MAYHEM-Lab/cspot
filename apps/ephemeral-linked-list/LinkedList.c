#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "AccessPointer.h"
#include "Data.h"
#include "Link.h"
#include "LinkedList.h"
#include "Helper.h"

#define LOG_ENABLED 0

FILE *fp;

char AP_WOOF_NAME[255];
unsigned long AP_WOOF_SIZE;
char DATA_WOOF_NAME[255];
unsigned long DATA_WOOF_SIZE;
unsigned long LINK_WOOF_SIZE;
int LINK_WOOF_NAME_SIZE;
char LOG_FILENAME[256];
unsigned long VERSION_STAMP;

void LL_init(
        int num_of_extra_links,
        unsigned long ap_woof_size,
        unsigned long data_woof_size,
        unsigned long link_woof_size
        ){
    WooFInit();
    srand(time(0));
    LINK_WOOF_NAME_SIZE = 20;
    strcpy(AP_WOOF_NAME, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    AP_WOOF_SIZE = ap_woof_size;
    createWooF(AP_WOOF_NAME, sizeof(AP), AP_WOOF_SIZE);
    strcpy(DATA_WOOF_NAME, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    DATA_WOOF_SIZE = data_woof_size;
    createWooF(DATA_WOOF_NAME, sizeof(DATA), DATA_WOOF_SIZE);
    LINK_WOOF_SIZE = link_woof_size;
    VERSION_STAMP = 0;
    #if LOG_ENABLED
        strcpy(LOG_FILENAME, "remote-result.log");
        fp = fopen(LOG_FILENAME, "w");
        fclose(fp);
        fp = NULL;
    #endif
}

void search(DI di, AP *node){

    AP iterator;
    DATA data;
    LINK current_link;
    int get_status;
    unsigned long latest_seq;

    iterator.dw_seq_no = 0;

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(latest_seq > 0){
        WooFGet(AP_WOOF_NAME, (void *)&iterator, latest_seq);
    }
    if(latest_seq == 0 || iterator.dw_seq_no == 0){
        //empty tree
        node->dw_seq_no = 0;
        return;
    }

    iterator.dw_seq_no = 0;
    get_status = WooFGet(AP_WOOF_NAME, (void *)&iterator, WooFGetLatestSeqno(AP_WOOF_NAME));

    while(1){
        get_status = WooFGet(DATA_WOOF_NAME, (void *)&data, iterator.dw_seq_no);
        if(data.di.val == di.val){
            node->dw_seq_no = iterator.dw_seq_no;
            return;
        }
        WooFGet(data.lw_name, (void *)&current_link, WooFGetLatestSeqno(data.lw_name));
        iterator.dw_seq_no = current_link.dw_seq_no;
        if(iterator.dw_seq_no == 0){//end of line
            node->dw_seq_no = 0;
            return;
        }
    }

}

void populate_terminal_node(AP *terminal_node){

    AP head;
    DATA data;
    LINK current_link;
    unsigned long latest_seq;

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "populate_terminal_node START\n");
        }
    #endif

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    head.dw_seq_no = 0;
    if(latest_seq > 0){
        WooFGet(AP_WOOF_NAME, (void *)&head, latest_seq);
    }
    if(latest_seq == 0 || head.dw_seq_no == 0){
        terminal_node->dw_seq_no = 0;

        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "populate_terminal_node END\n");
            }
        #endif
        return;
    }
    WooFGet(AP_WOOF_NAME, (void *)&head, latest_seq);

    while(1){
        WooFGet(DATA_WOOF_NAME, (void *)&data, head.dw_seq_no);
        WooFGet(data.lw_name, (void *)&current_link, WooFGetLatestSeqno(data.lw_name));
        if(current_link.dw_seq_no == 0) break;
        head.dw_seq_no = current_link.dw_seq_no;
    }

    terminal_node->dw_seq_no = head.dw_seq_no;

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "populate_terminal_node END\n");
        }
    #endif
}

void LL_insert(DI di){

    unsigned long ndx;
    unsigned long status;
    unsigned long latest_seq;
    AP ap;
    AP terminal_node;
    DATA data;
    DATA terminal_node_data;
    LINK link;

    ap.dw_seq_no = 0;
    search(di, &ap);
    if(ap.dw_seq_no != 0){
        //already present
        return;
    }

    VERSION_STAMP += 1;

    #if LOG_ENABLED
        fprintf(stdout, "1\n");
        fflush(stdout);
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "INSERT START\n");
            fprintf(fp, "DATA:new data\n");
        }
    #endif

    data.di = di;
    strcpy(data.lw_name, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    strcpy(data.pw_name, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    ndx = insertIntoWooF(DATA_WOOF_NAME, NULL, (void *)&data);
    status = WooFCreate(data.lw_name, sizeof(LINK), LINK_WOOF_SIZE);
    status = WooFCreate(data.pw_name, sizeof(LINK), LINK_WOOF_SIZE);

    populate_terminal_node(&terminal_node);
    if(terminal_node.dw_seq_no == 0){
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "LINK:new child\n");
                fprintf(fp, "LINK:new parent\n");
            }
        #endif

        //empty tree
        link.dw_seq_no = 0;
        link.type = 'T';
        insertIntoWooF(data.lw_name, NULL, (void *)&link);
        link.type = 'P';
        insertIntoWooF(data.pw_name, NULL, (void *)&link);
        ap.dw_seq_no = ndx;
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);

        #if LOG_ENABLED
            fprintf(fp, "INSERT END\n");
            fflush(fp);
            fclose(fp);
            fp = NULL;
        #endif
        return;
    }

    //terminal node to new node link
    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "DATA:parent data\n");
            fprintf(fp, "LINK:new child\n");
            fprintf(fp, "LINK:new parent\n");
            fprintf(fp, "LINK:new child\n");
        }
    #endif
    WooFGet(DATA_WOOF_NAME, (void *)&terminal_node_data, terminal_node.dw_seq_no);
    link.dw_seq_no = ndx;
    link.type = 'T';
    insertIntoWooF(terminal_node_data.lw_name, NULL, (void *)&link);
    //new node to terminal node link
    link.dw_seq_no = terminal_node.dw_seq_no;
    link.type = 'P';
    insertIntoWooF(data.pw_name, NULL, (void *)&link);
    //new node to null node link
    link.dw_seq_no = 0;
    link.type = 'L';
    insertIntoWooF(data.lw_name, NULL, (void *)&link);

    #if LOG_ENABLED
        fprintf(fp, "INSERT END\n");
        fflush(fp);
        fclose(fp);
        fp = NULL;
    #endif
}


void LL_delete(DI di){

    AP node;
    AP parent;
    AP child;
    DATA data;
    DATA parent_data;
    DATA child_data;
    LINK link;
    unsigned long latest_seq;

    /* find node containing di */
    node.dw_seq_no = 0;
    search(di, &node);

    /* node not found */
    if(node.dw_seq_no == 0){
        return;
    }

    VERSION_STAMP += 1;

    #if LOG_ENABLED
        fprintf(stdout, "1\n");
        fflush(stdout);
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "DELETE START\n");
            fprintf(fp, "DATA:target data\n");
            fprintf(fp, "LINK:get parent\n");
            fprintf(fp, "LINK:get child\n");
        }
    #endif

    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//all info of target woof
    WooFGet(data.pw_name, (void *)&link, WooFGetLatestSeqno(data.pw_name));//latest parent of target
    parent.dw_seq_no = link.dw_seq_no;
    WooFGet(data.lw_name, (void *)&link, WooFGetLatestSeqno(data.lw_name));//latest child of target
    child.dw_seq_no = link.dw_seq_no;

    if(parent.dw_seq_no == 0 && child.dw_seq_no == 0){
        node.dw_seq_no = 0;
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&node);
        return;
    }

    if(parent.dw_seq_no == 0){//child is the new head
        if(child.dw_seq_no != 0){
#if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "DATA:child data\n");
            }
#endif
            WooFGet(DATA_WOOF_NAME, (void *)&child_data, child.dw_seq_no);
            link.dw_seq_no = parent.dw_seq_no;
            link.type = 'P';
            insertIntoWooF(child_data.pw_name, NULL, (void *)&link);
        }
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&child);
    }else{
#if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "DATA:parent data\n");
            fprintf(fp, "LINK:new child\n");
        }
#endif
        //parent to child
        WooFGet(DATA_WOOF_NAME, (void *)&parent_data, parent.dw_seq_no);
        link.dw_seq_no = child.dw_seq_no;
        link.type = 'T';
        insertIntoWooF(parent_data.lw_name, NULL, (void *)&link);
        //child to parent
        if(child.dw_seq_no != 0){
#if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "DATA:child data\n");
                fprintf(fp, "LINK:new parent\n");
            }
#endif
            WooFGet(DATA_WOOF_NAME, (void *)&child_data, child.dw_seq_no);
            link.dw_seq_no = parent.dw_seq_no;
            link.type = 'P';
            insertIntoWooF(child_data.pw_name, NULL, (void *)&link);
        }
    }

    #if LOG_ENABLED
        fprintf(fp, "DELETE END\n");
        fflush(fp);
        fclose(fp);
        fp = NULL;
    #endif

}

void LL_print(){

    AP node;
    DATA data;
    LINK current_link;

    WooFGet(AP_WOOF_NAME, (void *)&node, WooFGetLatestSeqno(AP_WOOF_NAME));//head
    fprintf(stdout, "%lu: ", VERSION_STAMP);
    if(node.dw_seq_no == 0){
        fprintf(stdout, "\n");
        fflush(stdout);
        return;
    }
    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//info of head
    fprintf(stdout, "%d ", data.di.val);
    fflush(stdout);
    WooFGet(data.lw_name, (void *)&current_link, WooFGetLatestSeqno(data.lw_name));
    while(1){
        if(current_link.dw_seq_no == 0) break;
        node.dw_seq_no = current_link.dw_seq_no;
        WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//info of node
        WooFGet(data.lw_name, (void *)&current_link, WooFGetLatestSeqno(data.lw_name));
        fprintf(stdout, "%d ", data.di.val);
        fflush(stdout);
    }

    fprintf(stdout, "\n");
    fflush(stdout);

}

void debug_LINK(char *name){

    unsigned long i;
    LINK link;

    fprintf(stdout, "********************\n");
    fprintf(stdout, "DEBUGGING LINK %s\n", name);
    for(i = 1; i <= WooFGetLatestSeqno(name); ++i){
        WooFGet(name, (void *)&link, i);
        fprintf(stdout, "%lu: %lu\n", i, link.dw_seq_no);
    }
    fflush(stdout);

}

void debug_DATA(){

    unsigned long i;
    DATA data;

    fprintf(stdout, "********************\n");
    fprintf(stdout, "DEBUGGING DATA\n");
    for(i = 1; i <= WooFGetLatestSeqno(DATA_WOOF_NAME); ++i){
        WooFGet(DATA_WOOF_NAME, (void *)&data, i);
        fprintf(stdout, "%lu: %d %s %s\n", i, data.di.val, data.lw_name, data.pw_name);
    }
    fflush(stdout);

}

void debug_AP(){

    unsigned long i;
    AP ap;

    fprintf(stdout, "********************\n");
    fprintf(stdout, "DEBUGGING AP\n");
    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        WooFGet(AP_WOOF_NAME, (void *)&ap, i);
        fprintf(stdout, "%lu: %lu\n", i, ap.dw_seq_no);
    }
    fflush(stdout);

}

void LL_debug(){
    LL_print();
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

    fprintf(stdout, "%d,%zu\n", num_ops_input, total_size);
    fflush(stdout);

}
