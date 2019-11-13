#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "BST.h"
#include "DataItem.h"
#include "Data.h"

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    FILE *fp;
    int op;
    char val;
    int num_of_operations;
    int line_number;
    DATA debug_data;
    unsigned long found_at_seqno;
    unsigned long test_version_stamp;

    /* initialize bst */
    WooFInit();
    BST_init(1, "AP", 1000, "DATA", 20000, 20000);   

    /* data item has three fields: key (256 bytes), logId (int 64), and recordIdx (int 64) */
    di.logId = 1;
    di.recordIdx = 1;

    /* insert a bunch of data items by repeatedly changing key of di, S3_KEY_SIZE is set to 256 */
    strncpy(di.key, "hello", S3_KEY_SIZE);
    BST_insert(di);
    strncpy(di.key, "arsenal", S3_KEY_SIZE);
    BST_insert(di);
    strncpy(di.key, "foobar", S3_KEY_SIZE);
    BST_insert(di);

    /* preorder traversal at each version */
    BST_preorder();

    /* search for a key in the latest version (if not found will return 0, otherwise seqno in data woof) */
    found_at_seqno = BST_search_latest(di);
    if(found_at_seqno != 0){
        fprintf(stdout, "key {%.*s} FOUND at latest version\n", S3_KEY_SIZE, di.key);
    }else{
        fprintf(stdout, "key {%.*s} NOT FOUND at latest version\n", S3_KEY_SIZE, di.key);
    }


    /* search for a key in the latest version (if not found will return 0, otherwise seqno in data woof) */
    strncpy(di.key, "whatever", S3_KEY_SIZE);
    found_at_seqno = BST_search_latest(di);
    if(found_at_seqno != 0){
        fprintf(stdout, "key {%.*s} FOUND at latest version\n", S3_KEY_SIZE, di.key);
    }else{
        fprintf(stdout, "key {%.*s} NOT FOUND at latest version\n", S3_KEY_SIZE, di.key);
    }

    return(0);
}

