#include "dht.h"
#include "woofc-host.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    if (dht_create_woofs() < 0) {
        fprintf(stderr, "failed to initialize the cluster: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
