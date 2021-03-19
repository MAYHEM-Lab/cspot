#include "mqttc_test.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int err;
    WooFInit();
    err = WooFCreate(MQTTC_WOOFNAME, sizeof(MQTTC_TEST_EL), 5);
    if (err < 0) {
        fprintf(stderr, "couldn't create woof\n");
        fflush(stderr);
        exit(1);
    }

    return (0);
}
