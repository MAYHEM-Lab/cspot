#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "raft.h"
#include "raft_client.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char WooF_dir[2048];

extern int PUT_HANDLER_NAME(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr);

int get_client_addr(char* client_addr) {
    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        sprintf(dht_error_msg, "failed to get the latest dht node info: %s", dht_error_msg);
        return -1;
    }
    int i;
    for (i = strlen("woof://"); i < strlen(node.addr); ++i) {
        if (node.addr[i] == '/') {
            client_addr[i - strlen("woof://")] = '\0';
            return 0;
        }
        client_addr[i - strlen("woof://")] = node.addr[i];
    }
    sprintf(dht_error_msg, "couldn't find the last \"/\" in %s", node.addr);
    return -1;
}

unsigned long map_element(unsigned long raft_index) {
    DHT_MAP_RAFT_INDEX_ARG arg = {0};
    arg.raft_index = raft_index;

    char map_index_woof[DHT_NAME_LENGTH] = {0};
    sprintf(map_index_woof, "PUT_HANDLER_NAME_%s", DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);
    if (!WooFExist(map_index_woof)) {
        if (WooFCreate(map_index_woof, sizeof(DHT_MAP_RAFT_INDEX_ARG), DHT_HISTORY_LENGTH_LONG) < 0) {
            fprintf(stderr, "failed to create woof %s", map_index_woof);
            return -1;
        }
        char woof_file[DHT_NAME_LENGTH] = {0};
        sprintf(woof_file, "%s/%s", WooF_dir, map_index_woof);
        if (chmod(woof_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
            fprintf(stderr, "failed to change file %s's mode to 0666", map_index_woof);
            return -1;
        }
    }
    return WooFPut(map_index_woof, NULL, &arg);
}

int get_element(char* raft_addr, void* element, unsigned long seq_no) {
    char map_index_woof[DHT_NAME_LENGTH] = {0};
    sprintf(map_index_woof, "PUT_HANDLER_NAME_%s", DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);
    DHT_MAP_RAFT_INDEX_ARG arg = {0};
    if (WooFGet(map_index_woof, &arg, seq_no) < 0) {
        fprintf(stderr, "failed to get the index mapping at %lu from %s\n", seq_no, map_index_woof);
        return -1;
    }
    // RAFT_DATA_TYPE raft_data = {0};
    if (raft_get(raft_addr, element, arg.raft_index) < 0) {
        fprintf(stderr, "failed to get the raft_data at %lu from %s\n", arg.raft_index, raft_addr);
        return -1;
    }
    return 0;
}

int handler_wrapper(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_TRIGGER_ARG* arg = (DHT_TRIGGER_ARG*)ptr;

    log_set_tag("PUT_HANDLER_NAME");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    char client_addr[DHT_NAME_LENGTH];
    if (get_client_addr(client_addr) < 0) {
        log_error("failed to get client_addr: %s", dht_error_msg);
        exit(1);
    }
    dht_set_client_ip(client_addr);

    // log_debug("using raft, getting index %" PRIu64 " from %s", arg->seq_no, arg->woof_name);
    RAFT_DATA_TYPE raft_data = {0};
    while (raft_get(arg->element_woof, &raft_data, arg->element_seqno) < 0) {
        log_warn("failed to get raft data from %s at %lu, probably not committed yet", arg->element_woof, arg->element_seqno);
    }
    unsigned long mapped_seqno = map_element(arg->element_seqno);
    if (WooFInvalid(mapped_seqno)) {
        log_error("failed to map raft_index to seq_no");
        exit(1);
    }
    log_debug("mapped raft_index %lu to seq_no %lu", arg->element_seqno, mapped_seqno);
    int err = PUT_HANDLER_NAME(arg->element_woof, arg->topic_name, mapped_seqno, raft_data.val);

    return err;
}
