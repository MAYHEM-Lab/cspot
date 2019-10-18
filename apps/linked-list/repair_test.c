#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"

#include "LinkedList.h"
#include "DataItem.h"
#include "Data.h"

int main(int argc, char **argv)
{
    DI di;
    unsigned long i;
    AP node;
    Hval hval;
    Dlist *seqno;
    int err;
    DATA data;
    unsigned long seq;

    LL_init(1, 100, 100, 100);

    di.val = 2;LL_insert(di);
    di.val = 5;LL_insert(di);
    di.val = 7;LL_insert(di);
    di.val = 9;LL_insert(di);
    di.val = 7;LL_delete(di);

    for(i = 1; i <= 5; ++i){
        LL_print(i);
    }

    // search value 5 with version number 3
    di.val = 5;
    search(3, di, &node);
    printf("dw_seq_no: %lu, lw_seq_no: %lu\n", node.dw_seq_no, node.lw_seq_no);
    // get the DATA element by the dw_seq_no
    err = WooFGet("DATA", &data, node.dw_seq_no);
    if (err < 0)
    {
        printf("couldn't get DataWooF[%lu]\n", node.dw_seq_no);
        return 0;
    }

    // put the dw_seq_no to the seqno(double-linkedlist)
    seqno = DlistInit();
    hval.i64 = node.dw_seq_no;
    DlistAppend(seqno, hval);
    // initiate the repair
    err = WooFRepair("DATA", seqno);
    if (err < 0)
    {
        printf("couldn't repair DataWooF[%lu]\n", node.dw_seq_no);
        return 0;
    }
    else
    {
        printf("repairing DataWooF[%lu], original value: %d\n", node.dw_seq_no, data.di.val);
    }

    // change the DATA value to 4
    data.di.val = 4;
    // put the DATA back. It'll fall into the original dw_seq_no
    seq = WooFPut("DATA", NULL, &data);
    if (WooFInvalid(seq))
    {
        printf("couldn't put repaired value to DataWooF\n");
        return 0;
    }
    else
    {
        // seq should be equal to dw_seq_no we got ealier
        printf("repaired DataWooF[%lu], new value: %d\n", seq, data.di.val);
    }
    
    printf("After repaired:\n");
    for(i = 1; i <= 5; ++i){
        LL_print(i);
    }

    return(0);
}

