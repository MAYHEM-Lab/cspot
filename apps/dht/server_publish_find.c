#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#define MAX_PUBLISH_SIZE 512

int resolve(DHT_NODE_INFO* node, unsigned long seq_no) {
    DHT_SERVER_PUBLISH_FIND_ARG find_arg = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_FIND_WOOF, &find_arg, seq_no) < 0) {
        log_error("failed to get find_arg at %lu", seq_no);
        return -1;
    }
    if (find_arg.element_size > RAFT_DATA_TYPE_SIZE) {
        log_error("element_size %lu exceeds RAFT_DATA_TYPE_SIZE %lu", find_arg.element_size, RAFT_DATA_TYPE_SIZE);
        return -1;
    }

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, find_arg.topic_name);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, find_arg.topic_name, hashed_key, node->addr);
    arg.action_seqno = seq_no;
    arg.action = DHT_ACTION_PUBLISH_FIND;
    arg.ts_a = find_arg.ts_a;
    arg.ts_b = get_milliseconds();
    arg.ts_c = 0;

    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke h_find_successor");
        return -1;
    }
    return 0;
}

int server_publish_find(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;
    log_set_tag("server_publish_find");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    uint64_t begin = get_milliseconds();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        WooFMsgCacheShutdown();
        exit(1);
    }

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_SERVER_PUBLISH_FIND_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_SERVER_PUBLISH_FIND_WOOF);
        WooFMsgCacheShutdown();
        exit(1);
    }
    int count = latest_seq - routine_arg->last_seqno;
    if (count > MAX_PUBLISH_SIZE) {
        log_warn("publish_find lag: %d", count);
        count = MAX_PUBLISH_SIZE;
    }
    if (count != 0) {
        log_debug("processing %lu publish_find", count);
    }

    int i;
    for (i = 0; i < count; ++i) {
        if (resolve(&node, routine_arg->last_seqno + 1 + i) < 0) {
            log_error("failed to process publish_find at %lu", routine_arg->last_seqno + 1 + i);
            WooFMsgCacheShutdown();
            exit(1);
        }
    }
    routine_arg->last_seqno += count;
    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_find", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next server_publish_find");
        WooFMsgCacheShutdown();
        exit(1);
    }
    // printf("handler server_publish_find took %lu\n", get_milliseconds() - begin);
    WooFMsgCacheShutdown();
    return 1;
}
