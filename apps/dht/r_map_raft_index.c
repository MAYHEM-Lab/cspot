#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>


extern char WooF_dir[2048];

int r_map_raft_index(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("r_map_raft_index");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_MAP_RAFT_INDEX_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(DHT_MAP_RAFT_INDEX_ARG)) < 0) {
        log_error("failed to call monitor_cast");
        exit(1);
    }

    char index_woof[DHT_NAME_LENGTH] = {0};
    sprintf(index_woof, "%s_%s", arg.topic_name, DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);

    if (!WooFExist(index_woof)) {
        if (WooFCreate(index_woof, sizeof(DHT_MAP_RAFT_INDEX_ARG), DHT_HISTORY_LENGTH_LONG) < 0) {
            log_error("failed to create woof %s", index_woof);
            exit(1);
        }
        log_debug("created raft_index_map woof %s", index_woof);
        char woof_file[DHT_NAME_LENGTH] = {0};
        sprintf(woof_file, "%s/%s", WooF_dir, index_woof);
        if (chmod(woof_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
            log_error("failed to change file %s's mode to 0666", index_woof);
        }
    }

    unsigned long seq = WooFPut(index_woof, NULL, &arg);
    if (WooFInvalid(seq)) {
        log_error("failed to store raft indext");
        exit(1);
    }
    log_debug("map raft_index %" PRIu64 " to topic %s seq_no %lu", arg.raft_index, arg.topic_name, seq);

    monitor_exit(ptr);
    return 1;
}
