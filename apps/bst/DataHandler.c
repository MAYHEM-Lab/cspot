#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "Data.h"
#include "Link.h"
#include "DataItem.h"
#include "Helper.h"


int DataHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

    LINK link;


    DATA *el = (DATA *)ptr;
    fprintf(stdout,"DATA HANDLER -- WooF Name {%s} Seq No {%lu} Data {%s} Link WooF {%s} Parent WooF {%s}\n",
                    wf->shared->filename, seq_no, DI_get_str(el->di), el->lw_name, el->pw_name);
    fflush(stdout);
    WooFCreate(el->lw_name, sizeof(LINK), 100);
    WooFCreate(el->pw_name, sizeof(LINK), 100);
    link.dw_seq_no = 0;
    link.lw_seq_no = 0;
    link.version_stamp = el->version_stamp;
    link.type = 'L';
    WooFPut(el->lw_name, "LinkHandler", (void *)&link);
    link.type = 'R';
    WooFPut(el->lw_name, "LinkHandler", (void *)&link);
    link.type = 'P';
    WooFPut(el->pw_name, "LinkHandler", (void *)&link);
    return(1);

}

