#include "dht.h"
#include "monitor.h"
#include "woofc-host.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    if (dht_create_woofs() < 0) {
        fprintf(stderr, "failed to initialize the cluster: %s\n", dht_error_msg);
        exit(1);
    }

    if (monitor_create(DHT_MONITOR_NAME) < 0) {
        fprintf(stderr, "Failed to create and start the handler monitor\n");
        return -1;
    }

    return 0;
}
