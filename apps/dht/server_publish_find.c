#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

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
    arg.created_ts = get_milliseconds();

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

    uint64_t begin = get_milliseconds();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_SERVER_PUBLISH_FIND_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_SERVER_PUBLISH_FIND_WOOF);
        exit(1);
    }
    if (latest_seq != routine_arg->last_seqno) {
        log_debug("processing %lu publish_find", latest_seq - routine_arg->last_seqno);
    }

    unsigned long i;
    for (i = routine_arg->last_seqno + 1; i <= latest_seq; ++i) {
        if (resolve(&node, i) < 0) {
            log_error("failed to process publish_find at %lu", i);
            exit(1);
        }
    }
    routine_arg->last_seqno = latest_seq;
    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_find", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next server_publish_find");
        exit(1);
    }
    // printf("handler server_publish_find took %lu\n", get_milliseconds() - begin);
    return 1;
}
