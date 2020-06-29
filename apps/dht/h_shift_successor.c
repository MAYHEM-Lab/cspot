#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_shift_successor(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("shift_successor");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    int i;
    for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i) {
        memcpy(successor.hash[i], successor.hash[i + 1], sizeof(successor.hash[i]));
        memcpy(successor.replicas[i], successor.replicas[i + 1], sizeof(successor.replicas[i]));
        successor.leader[i] = successor.leader[i + 1];
    }
    memset(successor.hash[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor.hash[DHT_SUCCESSOR_LIST_R - 1]));
    memset(successor.replicas[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor.replicas[DHT_SUCCESSOR_LIST_R - 1]));
    successor.leader[DHT_SUCCESSOR_LIST_R - 1] = 0;

    unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
    if (WooFInvalid(seq)) {
        log_error("failed to shift successor list");
        monitor_exit(ptr);
        exit(1);
    }
    log_debug("shifted successor list");

    monitor_exit(ptr);
    return 1;
}
