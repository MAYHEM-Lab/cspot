#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "AccessPointer.h"
#include "Data.h"
#include "Link.h"
#include "LinkedList.h"
#include "Helper.h"
#include "CheckPointer.h"

#define CHECKPOINT_ENABLED 0
#define LOG_ENABLED 1

FILE *fp;

int NUM_OF_EXTRA_LINKS;
unsigned long VERSION_STAMP;
char AP_WOOF_NAME[255];
unsigned long AP_WOOF_SIZE;
char DATA_WOOF_NAME[255];
unsigned long DATA_WOOF_SIZE;
unsigned long LINK_WOOF_SIZE;
int LINK_WOOF_NAME_SIZE;
int NUM_OF_LINKS_PER_NODE;
int CHECKPOINT_MAX_ELEMENTS;
char LOG_FILENAME[256];

void LL_init(
        int num_of_extra_links,
        unsigned long ap_woof_size,
        unsigned long data_woof_size,
        unsigned long link_woof_size
        ){
    WooFInit();
    srand(time(0));
    LINK_WOOF_NAME_SIZE = 20;
    NUM_OF_EXTRA_LINKS = num_of_extra_links;
    VERSION_STAMP = 0;
    strcpy(AP_WOOF_NAME, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    AP_WOOF_SIZE = ap_woof_size;
    createWooF(AP_WOOF_NAME, sizeof(AP), AP_WOOF_SIZE);
    strcpy(DATA_WOOF_NAME, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    DATA_WOOF_SIZE = data_woof_size;
    createWooF(DATA_WOOF_NAME, sizeof(DATA), DATA_WOOF_SIZE);
    LINK_WOOF_SIZE = link_woof_size;
    NUM_OF_LINKS_PER_NODE = 1 + NUM_OF_EXTRA_LINKS;
    CHECKPOINT_MAX_ELEMENTS = 8;
    CP_init(1 + CHECKPOINT_MAX_ELEMENTS * (LINK_WOOF_NAME_SIZE + sizeof(unsigned long)), getRandomWooFName(LINK_WOOF_NAME_SIZE), LINK_WOOF_NAME_SIZE, ap_woof_size);
    #if LOG_ENABLED
        strcpy(LOG_FILENAME, "remote-result.log");
        fp = fopen(LOG_FILENAME, "w");
        fclose(fp);
        fp = NULL;
    #endif
}

void add_node(unsigned long version_stamp, AP parent, AP child){

    DATA parent_data;
    DATA child_data;
    LINK link;
    LINK grandparent_link;
    AP node;
    unsigned long ndx;
    unsigned long latest_seq;

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "add_node START\n");
        }
    #endif

    if(parent.dw_seq_no == 0){//parent null
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "DATA:get child\n");
            }
        #endif
        node.dw_seq_no = child.dw_seq_no;
        node.lw_seq_no = child.lw_seq_no;
        WooFGet(DATA_WOOF_NAME, (void *)&child_data, child.dw_seq_no);
        link.dw_seq_no = 0;
        link.lw_seq_no = 0;
        link.type = 'P';
        link.version_stamp = version_stamp;
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "LINK:new parent\n");
            }
        #endif
        insertIntoWooF(child_data.pw_name, NULL, (void *)&link);
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&node);
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "add_node END\n");
            }
        #endif
        return;
    }

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "DATA:get parent\n");
        }
    #endif
    WooFGet(DATA_WOOF_NAME, (void *)&parent_data, parent.dw_seq_no);
    if(child.dw_seq_no != 0){
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "DATA:get child\n");
            }
        #endif
        WooFGet(DATA_WOOF_NAME, (void *)&child_data, child.dw_seq_no);
    }

    //parent to child link
    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "LINK:new child\n");
        }
    #endif
    link.version_stamp = version_stamp;
    link.dw_seq_no = child.dw_seq_no;
    link.lw_seq_no = child.lw_seq_no;
    link.type = 'T';
    ndx = insertIntoWooF(parent_data.lw_name, NULL, (void *)&link);

    //child to parent link
    if(child.dw_seq_no != 0){
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "LINK:new parent\n");
            }
        #endif
        link.dw_seq_no = parent.dw_seq_no;
        link.lw_seq_no = ndx - ((ndx % NUM_OF_LINKS_PER_NODE == 0) ? NUM_OF_LINKS_PER_NODE : (ndx % NUM_OF_LINKS_PER_NODE)) + 1;
        insertIntoWooF(child_data.pw_name, NULL, (void *)&link);
    }

    //overflow
    if(ndx % NUM_OF_LINKS_PER_NODE == 1){
        #if LOG_ENABLED
            if(fp != NULL){
                fprintf(fp, "LINK:get grandparent\n");
            }
        #endif
        WooFGet(parent_data.pw_name, (void *)&grandparent_link, WooFGetLatestSeqno(parent_data.pw_name));
        child.dw_seq_no = parent.dw_seq_no;
        child.lw_seq_no = ndx;
        parent.dw_seq_no = grandparent_link.dw_seq_no;
        parent.lw_seq_no = grandparent_link.lw_seq_no;
        add_node(version_stamp, parent, child);
    }

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "add_node END\n");
        }
    #endif
}

void populate_current_link(unsigned long version_stamp, AP node, LINK *current_link){

    DATA data;
    LINK link;
    unsigned long i;
    unsigned long last_seq;
    unsigned long max_vs_seen;

    #if LOG_ENABLED
        if(fp != NULL){   
            fprintf(fp, "populate_current_link START\n");
            fprintf(fp, "DATA:get node\n");
        }
    #endif
    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//need link woof name
    last_seq = WooFGetLatestSeqno(data.lw_name);
    max_vs_seen = 0;
    for(i = 0; i < NUM_OF_LINKS_PER_NODE; ++i){//traverse through all links of that woof
        if((node.lw_seq_no + i) <= last_seq){
            #if LOG_ENABLED
                if(fp != NULL){
                    fprintf(fp, "LINK:get child\n");
                }
            #endif
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

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "populate_current_link END\n");
        }
    #endif

}

void populate_terminal_node(AP *terminal_node){

    AP head;
    LINK current_link;

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "populate_terminal_node START\n");
        }
    #endif

    WooFGet(AP_WOOF_NAME, (void *)&head, VERSION_STAMP);

    while(1){
        populate_current_link(VERSION_STAMP, head, &current_link);
        if(current_link.dw_seq_no == 0) break;
        head.dw_seq_no = current_link.dw_seq_no;
        head.lw_seq_no = current_link.lw_seq_no;
    }

    terminal_node->dw_seq_no = head.dw_seq_no;
    terminal_node->lw_seq_no = head.lw_seq_no;

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "populate_terminal_node END\n");
        }
    #endif
}

void LL_insert(DI di){

    unsigned long ndx;
    unsigned long cp_ndx;
    unsigned long status;
    unsigned long working_vs;
    unsigned long latest_seq;
    AP ap;
    AP terminal_node;
    DATA data;
    LINK link;

    #if CHECKPOINT_ENABLED
    char **WooF_names;
    unsigned long *seq_nos;
    int cp_num_of_elements;
    int cp_i;
    LINK cp_link;
    DATA cp_data;

    WooF_names = (char **)malloc(CHECKPOINT_MAX_ELEMENTS * sizeof(char *));
    for(cp_i = 0; cp_i < CHECKPOINT_MAX_ELEMENTS; ++cp_i){
        WooF_names[cp_i] = (char *)malloc(LINK_WOOF_NAME_SIZE * sizeof(char));
    }
    seq_nos = (unsigned long *)malloc(CHECKPOINT_MAX_ELEMENTS * sizeof(unsigned long));
    cp_num_of_elements = 0;
    #endif

    working_vs = VERSION_STAMP + 1;

    if(VERSION_STAMP != 0){
        ap.dw_seq_no = 0;
        ap.lw_seq_no = 0;
        search(VERSION_STAMP, di, &ap);
        if(ap.dw_seq_no != 0){//already present
            return;
        }
        ap.dw_seq_no = 0;
        ap.lw_seq_no = 0;
    }

    #if LOG_ENABLED
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "INSERT START:%lu\n", working_vs);
        }
    #endif

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "DATA:new data\n");
        }
    #endif
    data.di = di;
    strcpy(data.lw_name, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    strcpy(data.pw_name, getRandomWooFName(LINK_WOOF_NAME_SIZE));
    data.version_stamp = working_vs;
    ndx = insertIntoWooF(DATA_WOOF_NAME, NULL, (void *)&data);//insert into data woof, ndx needed for insertion into ap woof

    #if CHECKPOINT_ENABLED
    strcpy(WooF_names[cp_num_of_elements], DATA_WOOF_NAME);
    seq_nos[cp_num_of_elements] = ndx;
    cp_num_of_elements++;
    #endif

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "LINK:new child\n");
            fprintf(fp, "LINK:new parent\n");
        }
    #endif
    status = WooFCreate(data.lw_name, sizeof(LINK), LINK_WOOF_SIZE);
    status = WooFCreate(data.pw_name, sizeof(LINK), LINK_WOOF_SIZE);
    link.dw_seq_no = 0;//points to null, for both parent and child
    link.lw_seq_no = 0;
    link.version_stamp = data.version_stamp;
    link.type = 'T';//to or next
    cp_ndx = insertIntoWooF(data.lw_name, NULL, (void *)&link);//insert into link woof

    #if CHECKPOINT_ENABLED
    strcpy(WooF_names[cp_num_of_elements], data.lw_name);
    seq_nos[cp_num_of_elements] = cp_ndx;
    cp_num_of_elements++;
    #endif

    link.type = 'P';//parent
    cp_ndx = insertIntoWooF(data.pw_name, NULL, (void *)&link);//insert into parent woof, note add_node changes it

    #if CHECKPOINT_ENABLED
    strcpy(WooF_names[cp_num_of_elements], data.pw_name);
    seq_nos[cp_num_of_elements] = cp_ndx;
    cp_num_of_elements++;
    #endif

    ap.dw_seq_no = ndx;
    ap.lw_seq_no = 1;

    if(WooFGetLatestSeqno(AP_WOOF_NAME) > 0){//there is at least one element in linked list
        populate_terminal_node(&terminal_node);
    }else{//empty linked list
        cp_ndx = insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);

        #if CHECKPOINT_ENABLED
        strcpy(WooF_names[cp_num_of_elements], AP_WOOF_NAME);
        seq_nos[cp_num_of_elements] = cp_ndx;
        cp_num_of_elements++;
        CP_write(cp_num_of_elements, WooF_names, seq_nos);
        #endif

        VERSION_STAMP = working_vs;
        #if LOG_ENABLED
            fprintf(fp, "INSERT END:%lu\n", working_vs);
            fflush(fp);
            fclose(fp);
            fp = NULL;
        #endif
        return;
    }

    add_node(working_vs, terminal_node, ap);

    #if CHECKPOINT_ENABLED
    WooFGet(DATA_WOOF_NAME, (void *)&cp_data, terminal_node.dw_seq_no);
    strcpy(WooF_names[cp_num_of_elements], cp_data.lw_name);
    seq_nos[cp_num_of_elements] = WooFGetLatestSeqno(cp_data.lw_name);
    cp_num_of_elements++;
    WooFGet(DATA_WOOF_NAME, (void *)&cp_data, ap.dw_seq_no);
    strcpy(WooF_names[cp_num_of_elements], cp_data.pw_name);
    seq_nos[cp_num_of_elements] = WooFGetLatestSeqno(cp_data.pw_name);
    cp_num_of_elements++;
    #endif

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(latest_seq < working_vs){//latest entry has not been made to AP WooF
        WooFGet(AP_WOOF_NAME, (void *)&ap, latest_seq);
        cp_ndx = insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&ap);

        #if CHECKPOINT_ENABLED
        strcpy(WooF_names[cp_num_of_elements], AP_WOOF_NAME);
        seq_nos[cp_num_of_elements] = cp_ndx;
        cp_num_of_elements++;
        #endif
    }

    #if CHECKPOINT_ENABLED
    CP_write(cp_num_of_elements, WooF_names, seq_nos);
    #endif

    VERSION_STAMP = working_vs;

    #if LOG_ENABLED
        fprintf(fp, "INSERT END:%lu\n", working_vs);
        fflush(fp);
        fclose(fp);
        fp = NULL;
    #endif
}

void search(unsigned long version_stamp, DI di, AP *node){

    AP iterator;
    DATA data;
    LINK current_link;
    int get_status;

    get_status = WooFGet(AP_WOOF_NAME, (void *)&iterator, version_stamp);

    while(1){
        get_status = WooFGet(DATA_WOOF_NAME, (void *)&data, iterator.dw_seq_no);
        if(data.di.val == di.val){
            node->dw_seq_no = iterator.dw_seq_no;
            node->lw_seq_no = iterator.lw_seq_no;
            return;
        }
        populate_current_link(version_stamp, iterator, &current_link);
        iterator.dw_seq_no = current_link.dw_seq_no;
        iterator.lw_seq_no = current_link.lw_seq_no;
        if(iterator.dw_seq_no == 0){//end of line
            node->dw_seq_no = 0;
            node->lw_seq_no = 0;
            return;
        }
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

    #if LOG_ENABLED
        fp = fopen(LOG_FILENAME, "a");
        if(fp != NULL){
            fprintf(fp, "DELETE START:%lu\n", working_vs);
        }
    #endif

    #if LOG_ENABLED
        if(fp != NULL){
            fprintf(fp, "DATA:target data\n");
            fprintf(fp, "LINK:get parent\n");
        }
    #endif
    WooFGet(DATA_WOOF_NAME, (void *)&data, node.dw_seq_no);//all info of target woof
    WooFGet(data.pw_name, (void *)&link, WooFGetLatestSeqno(data.pw_name));//latest parent of target
    parent.dw_seq_no = link.dw_seq_no;
    parent.lw_seq_no = link.lw_seq_no;
    populate_current_link(VERSION_STAMP, node, &link);//latest child of target
    child.dw_seq_no = link.dw_seq_no;
    child.lw_seq_no = link.lw_seq_no;

    add_node(working_vs, parent, child);

    latest_seq = WooFGetLatestSeqno(AP_WOOF_NAME);
    if(working_vs > latest_seq){
        WooFGet(AP_WOOF_NAME, (void *)&node, latest_seq);
        insertIntoWooF(AP_WOOF_NAME, NULL, (void *)&node);
    }
    VERSION_STAMP = working_vs;

    #if LOG_ENABLED
        fprintf(fp, "DELETE END:%lu\n", working_vs);
        fflush(fp);
        fclose(fp);
        fp = NULL;
    #endif

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
