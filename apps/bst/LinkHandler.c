#include <stdio.h>

#include "woofc.h"
#include "Link.h"

int LinkHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

    LINK *el = (LINK *)ptr;
    fprintf(stdout,"LINK HANDLER -- WooF Name {%s} Seq No {%lu} DW Seq {%lu} LW Seq {%lu} Type {%c}\n",
                    wf->shared->filename, seq_no, el->dw_seq_no, el->lw_seq_no, el->type);
    fflush(stdout);
    return(1);

}

