#include <stdio.h>

#include "Link.h"

void LINK_display(LINK link){

    fprintf(stdout, "Data WooF seq: %lu\n", link.dw_seq_no);
    fprintf(stdout, "Link WooF seq: %lu\n", link.lw_seq_no);
    fprintf(stdout, "Link type: %c\n", link.type);

}
