#include "mqttc_test.h"

#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int mqttc_test(WOOF* wf, unsigned long seq_no, void* ptr) {

    MQTTC_TEST_EL* el = (MQTTC_TEST_EL*)ptr;
    fprintf(stdout, "from woof %s at %lu with string: %s\n", wf->shared->filename, seq_no, el->string);
    fflush(stdout);
    return (1);
}
