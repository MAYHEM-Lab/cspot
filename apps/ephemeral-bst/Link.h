#ifndef LINK_H
#define LINK_H

struct Link{

    unsigned long dw_seq_no;
    char type;

};

typedef struct Link LINK;

void LINK_display(LINK link);

#endif
