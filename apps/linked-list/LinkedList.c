#include <stdio.h>
#include <string.h>
#include <time.h>

#include "AccessPointer.h"
#include "Data.h"
#include "Link.h"
#include "LinkedList.h"
#include "Helper.h"

int NUM_OF_EXTRA_LINKS;
unsigned long VERSION_STAMP;
char AP_WOOF_NAME[255];
unsigned long AP_WOOF_SIZE;
char DATA_WOOF_NAME[255];
unsigned long DATA_WOOF_SIZE;
unsigned long LINK_WOOF_SIZE;
int LINK_WOOF_NAME_SIZE;
int NUM_OF_LINKS_PER_NODE;

void LL_init(
        int num_of_extra_links,
        char *ap_woof_name,
        unsigned long ap_woof_size,
        char *data_woof_name,
        unsigned long data_woof_size,
        unsigned long link_woof_size
        ){
    WooFInit();
    srand(time(0));
    NUM_OF_EXTRA_LINKS = num_of_extra_links;
    VERSION_STAMP = 0;
    strcpy(AP_WOOF_NAME, ap_woof_name);
    AP_WOOF_SIZE = ap_woof_size;
    createWooF(AP_WOOF_NAME, sizeof(AP), AP_WOOF_SIZE);
    strcpy(DATA_WOOF_NAME, data_woof_name);
    DATA_WOOF_SIZE = data_woof_size;
    createWooF(DATA_WOOF_NAME, sizeof(DATA), DATA_WOOF_SIZE);
    LINK_WOOF_SIZE = link_woof_size;
    NUM_OF_LINKS_PER_NODE = 1 + NUM_OF_EXTRA_LINKS;
    LINK_WOOF_NAME_SIZE = 20;
}

void add_node(unsigned long version_stamp, AP parent, AP child){

    DATA parent_data;
    DATA child_data;
    LINK link;
    LINK grandparent_link;
    AP node;
    unsigned long ndx;
    unsigned long latest_seq;

    if(parent.dw_seq_no == 0){//parent null
        node.dw_seq_no = child.dw_seq_no;
        node.lw_seq_no = child.lw_seq_no;
        WooFGet(DATA_WOOF_NAME, (void *)&child_data, child.dw_seq_no);
        link.dw_seq_no = 0;
        link.lw_seq_no = 0;
        link.type = 'P';
        link.version_stamp = version_stamp;
        insertIntoWooF(child_data.pw_name, NULL, (void *)&link);
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&node);
        return;
    }

    WooFGet(DATA_WOOF_NAME, (void *)&parent_data, parent.dw_seq_no);
    if(child.dw_seq_no != 0){
        WooFGet(DATA_WOOF_NAME, (void *)&child_data, child.dw_seq_no);
    }

    //parent to child link
    link.version_stamp = version_stamp;
    link.dw_seq_no = child.dw_seq_no;
    link.lw_seq_no = child.lw_seq_no;
    link.type = 'T';
    ndx = insertIntoWooF(parent_data.lw_name, NULL, (void *)&link);

    //child to parent link
    if(child.dw_seq_no != 0){
        link.dw_seq_no = parent.dw_seq_no;
        link.lw_seq_no = ndx - ((ndx % NUM_OF_LINKS_PER_NODE == 0) ? NUM_OF_LINKS_PER_NODE : (ndx % NUM_OF_LINKS_PER_NODE)) + 1;
        insertIntoWooF(child_data.pw_name, NULL, (void *)&link);
    }

    //overflow
    if(ndx % NUM_OF_LINKS_PER_NODE == 1){
        WooFGet(parent_data.pw_name, (void *)&grandparent_link, WooFGetLatestSeqno(parent_data.pw_name));
        child.dw_seq_no = parent.dw_seq_no;
        child.lw_seq_no = ndx;
        parent.dw_seq_no = grandparent_link.dw_seq_no;
        parent.lw_seq_no = grandparent_link.lw_seq_no;
        add_node(version_stamp, parent, child);
    }

}

void populate_current_link(unsigned long version_stamp, AP node, LINK *current_link){

    DATA data;
    LINK link;
    unsigned long i;
    unsigned long last_seq;
    unsigned long max_vs_seen;

    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//need link woof name
    last_seq = WooFGetLatestSeqno(data.lw_name);
    max_vs_seen = 0;
    for(i = 0; i < NUM_OF_LINKS_PER_NODE; ++i){//traverse through all links of that woof
        if((node.lw_seq_no + i) <= last_seq){
            WooFGet(data.lw_name, (void *)&link, node.lw_seq_no + i);
            if(link.version_stamp >= max_vs_seen && link.version_stamp <= version_stamp){
                max_vs_seen = link.version_stamp;
                current_link->version_stamp = link.version_stamp;
                current_link->dw_seq_no = link.dw_seq_no;
                current_link->lw_seq_no = link.lw_seq_no;
                current_link->type = 'T';
            }
        }
    }

}

void populate_terminal_node(AP *terminal_node){

    AP head;
    LINK current_link;

    WooFGet(AP_WOOF_NAME, (void *)&head, VERSION_STAMP);

    while(1){
        populate_current_link(VERSION_STAMP, head, &current_link);
        if(current_link.dw_seq_no == 0) break;
        head.dw_seq_no = current_link.dw_seq_no;
        head.lw_seq_no = current_link.lw_seq_no;
    }

    terminal_node->dw_seq_no = head.dw_seq_no;
    terminal_node->lw_seq_no = head.lw_seq_no;

}

void LL_insert(DI di){

    unsigned long ndx;
    unsigned long status;
    unsigned long working_vs;
    unsigned long latest_seq;
    AP ap;
    AP terminal_node;
    DATA data;
    LINK link;

    working_vs = VERSION_STAMP + 1;

    if(VERSION_STAMP != 0){
        ap.dw_seq_no = 0;
        ap.lw_seq_no = 0;
        search(VERSION_STAMP, di, &ap);
        if(ap.lw_seq_no != 0){//already present
            return;
        }
        ap.dw_seq_no = 0;
        ap.lw_seq_no = 0;
    }

    data.di = di;
    strcpy(data.lw_name, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    strcpy(data.pw_name, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    data.version_stamp = working_vs;
    ndx = insertIntoWooF(DATA_WOOF_NAME, NULL, (void *)&data);//insert into data woof, ndx needed for insertion into ap woof
    status = WooFCreate(data.lw_name, sizeof(LINK), LINK_WOOF_SIZE);
    status = WooFCreate(data.pw_name, sizeof(LINK), LINK_WOOF_SIZE);
    link.dw_seq_no = 0;//points to null, for both parent and child
    link.lw_seq_no = 0;
    link.version_stamp = data.version_stamp;
    link.type = 'T';//to or next
    insertIntoWooF(data.lw_name, NULL, (void *)&link);//insert into link woof
    link.type = 'P';//parent
    insertIntoWooF(data.pw_name, NULL, (void *)&link);//insert into parent woof, note add_node changes it

    ap.dw_seq_no = ndx;
    ap.lw_seq_no = 1;

    if(WooFGetLatestSeqno(AP_WOOF_NAME) > 0){//there is at least one element in linked list
        populate_terminal_node(&terminal_node);
    }else{//empty linked list
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
        VERSION_STAMP = working_vs;
        return;
    }

    add_node(working_vs, terminal_node, ap);

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(latest_seq < working_vs){//latest entry has not been made to AP WooF
        WooFGet(AP_WOOF_NAME, (void *)&ap, latest_seq);
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);
    }
    VERSION_STAMP = working_vs;

}

void search(unsigned long version_stamp, DI di, AP *node){

    AP iterator;
    DATA data;
    LINK current_link;

    WooFGet(AP_WOOF_NAME, (void *)&iterator, version_stamp);

    while(1){
        WooFGet(DATA_WOOF_NAME, (void *)&data, iterator.dw_seq_no);
        if(data.di.val == di.val){
            node->dw_seq_no = iterator.dw_seq_no;
            node->lw_seq_no = iterator.lw_seq_no;
            return;
        }
        if(iterator.dw_seq_no == 0){//end of line
            node->dw_seq_no = 0;
            node->lw_seq_no = 0;
            return;
        }
        populate_current_link(version_stamp, iterator, &current_link);
        iterator.dw_seq_no = current_link.dw_seq_no;
        iterator.lw_seq_no = current_link.lw_seq_no;
    }

}

void LL_delete(DI di){

    unsigned long working_vs;
    AP node;
    AP parent;
    AP child;
    DATA data;
    LINK link;
    unsigned long latest_seq;
    DATA debug_data;

    /* trying to delete from empty tree */
    if(VERSION_STAMP == 0) return;

    working_vs = VERSION_STAMP + 1;

    /* find node containing di */
    node.dw_seq_no = 0;
    node.lw_seq_no = 0;
    search(VERSION_STAMP, di, &node);

    /* node not found */
    if(node.dw_seq_no == 0){
        return;
    }

    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//all info of target woof
    WooFGet(data.pw_name, (void *)&link, WooFGetLatestSeqno(data.pw_name));//latest parent of target
    parent.dw_seq_no = link.dw_seq_no;
    parent.lw_seq_no = link.lw_seq_no;
    populate_current_link(VERSION_STAMP, node, &link);//latest child of target
    child.dw_seq_no = link.dw_seq_no;
    child.lw_seq_no = link.lw_seq_no;

    //works till now
    add_node(working_vs, parent, child);

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(working_vs > latest_seq){
        WooFGet(AP_WOOF_NAME, (void *)&node, latest_seq);
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&node);
    }
    VERSION_STAMP = working_vs;

}

void LL_print(unsigned long version_stamp){

    AP node;
    DATA data;
    LINK current_link;

    WooFGet(AP_WOOF_NAME, (void *)&node, version_stamp);//head
    fprintf(stdout, "%lu: ", version_stamp);
    if(node.dw_seq_no == 0){
        fprintf(stdout, "\n");
        fflush(stdout);
        return;
    }
    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//info of head
    populate_current_link(version_stamp, node, &current_link);
    fprintf(stdout, "%d ", data.di.val);
    fflush(stdout);
    while(1){
        if(current_link.dw_seq_no == 0) break;
        node.dw_seq_no = current_link.dw_seq_no;
        node.lw_seq_no = current_link.lw_seq_no;
        WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//info of node
        populate_current_link(version_stamp, node, &current_link);
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
        fprintf(stdout, "%lu: %lu %lu %lu\n", i, link.version_stamp, link.dw_seq_no, link.lw_seq_no);
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
        fprintf(stdout, "%lu: %lu %d %s %s\n", i, data.di.val, data.version_stamp, data.lw_name, data.pw_name);
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
        fprintf(stdout, "%lu: %lu %lu\n", i, ap.dw_seq_no, ap.lw_seq_no);
    }
    fflush(stdout);

}

void LL_debug(){
    unsigned long i;
    for(i = 1; i <= WooFGetLatestSeqno(AP_WOOF_NAME); ++i){
        LL_print(i);
    }
}
