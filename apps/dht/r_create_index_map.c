#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char WooF_dir[2048];

int r_create_index_map(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_CREATE_INDEX_MAP_ARG* arg = (DHT_CREATE_INDEX_MAP_ARG*)ptr;
    log_set_tag("r_create_index_map");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    // create index_map woof
    char index_map_woof[DHT_NAME_LENGTH] = {0};
    sprintf(index_map_woof, "%s_%s", arg->topic_name, RAFT_INDEX_MAPPING_WOOF_SUFFIX);
    log_debug("creating %s", index_map_woof);
    if (WooFCreate(index_map_woof, sizeof(RAFT_INDEX_MAP), RAFT_WOOF_HISTORY_SIZE_LONG) < 0) {
        log_error("failed to create index_map woof %s", index_map_woof);
        WooFMsgCacheShutdown();
        exit(1);
    }
    char woof_file[DHT_NAME_LENGTH] = {0};
    sprintf(woof_file, "%s/%s", WooF_dir, index_map_woof);
    if (chmod(woof_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
        log_error("failed to change file %s's mode to 0666", index_map_woof);
        WooFMsgCacheShutdown();
        exit(1);
    }
    log_debug("created %s", index_map_woof);

    WooFMsgCacheShutdown();
    return 1;
}
