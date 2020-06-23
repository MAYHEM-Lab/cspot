#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int r_set_successor(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_SUCCESSOR_INFO* arg = (DHT_SUCCESSOR_INFO*)ptr;

    log_set_tag("set_successor");
    // log_set_level(DHT_LOG_DEBUG);
    log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

    unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, arg);
    if (WooFInvalid(seq)) {
        log_error("failed to set successor");
        exit(1);
    }
    log_debug("set successor to %s", successor_addr(arg, 0));

    return 1;
}
