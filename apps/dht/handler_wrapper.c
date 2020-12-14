#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-access.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft.h"
#include "raft_client.h"
#endif

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
    DHT_INVOCATION_ARG* arg = (DHT_INVOCATION_ARG*)ptr;

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

#ifdef USE_RAFT
    // log_debug("using raft, getting index %" PRIu64 " from %s", arg->seq_no, arg->woof_name);
    RAFT_DATA_TYPE raft_data = {0};
    if (raft_get(arg->woof_name, &raft_data, arg->seq_no) < 0) {
        log_error("failed to get raft data from %s at %" PRIu64 ": %s", arg->woof_name, arg->seq_no, raft_error_msg);
        exit(1);
    }
    unsigned long mapped_seqno = map_element(arg->seq_no);
    if (WooFInvalid(mapped_seqno)) {
        log_error("failed to map raft_index to seq_no");
        exit(1);
    }
    log_debug("mapped raft_index %lu to seq_no %lu", arg->seq_no, mapped_seqno);
    int err = PUT_HANDLER_NAME(arg->woof_name, arg->topic_name, mapped_seqno, raft_data.val);
#else
    log_debug("using woof, getting seqno %" PRIu64 " from %s", arg->seq_no, arg->woof_name);
    unsigned long element_size = WooFMsgGetElSize(arg->woof_name);
    if (WooFInvalid(element_size)) {
        log_error("failed to get element size of %s", arg->woof_name);
        exit(1);
    }

    void* element = malloc(element_size);
    if (element == NULL) {
        log_error("failed to malloc element with size %lu", element_size);
        exit(1);
    }
    if (WooFGet(arg->woof_name, element, arg->seq_no) < 0) {
        log_error("failed to read element from %s at %" PRIu64 "", arg->woof_name, arg->seq_no);
        free(element);
        exit(1);
    }

    int err = PUT_HANDLER_NAME(arg->woof_name, arg->topic_name, arg->seq_no, element);
    free(element);
#endif

    return err;
}
