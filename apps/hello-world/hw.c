#include "hw.h"

#include "woofc.h"

#include <stdio.h>

int hw(WOOF* wf, unsigned long seq_no, void* ptr) {
    HW_EL* el = (HW_EL*)ptr;
    fprintf(stdout, "hello world\n");
    fprintf(stdout, "at %lu with string: %s\n", seq_no, el->string);
    fflush(stdout);
    return (1);
}