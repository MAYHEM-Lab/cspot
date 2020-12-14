#include "dht.h"
#include "dht_utils.h"
#include "raft_client.h"
#include "string.h"
#include "woofc.h"

typedef struct resolve_thread_arg {
    char node_addr[DHT_NAME_LENGTH];
    unsigned long seq_no;
} RESOLVE_THREAD_ARG;

void* resolve_thread(void* arg) {
    RESOLVE_THREAD_ARG* thread_arg = (RESOLVE_THREAD_ARG*)arg;
    RAFT_CLIENT_PUT_RESULT put_result = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_MAP_WOOF, &put_result, thread_arg->seq_no) < 0) {
        log_error("failed to get put_result at %lu", thread_arg->seq_no);
        return;
    }

    DHT_TRIGGER_ARG partial_trigger_arg = {0};
    if (WooFGet(put_result.extra_woof, &partial_trigger_arg, put_result.extra_seqno) < 0) {
        log_error("failed to get partial_trigger_arg from %s at %lu", put_result.extra_woof, put_result.extra_seqno);
        return;
    }
    partial_trigger_arg.data = get_milliseconds();
    partial_trigger_arg.element_seqno = put_result.index;
    unsigned long trigger_seq = WooFPut(DHT_PARTIAL_TRIGGER_WOOF, NULL, &partial_trigger_arg);
    if (WooFInvalid(trigger_seq)) {
        log_error("failed to store partial trigger to %s", DHT_PARTIAL_TRIGGER_WOOF);
        return;
    }

    DHT_MAP_RAFT_INDEX_ARG map_raft_index_arg = {0};
    strcpy(map_raft_index_arg.topic_name, partial_trigger_arg.topic_name);
    map_raft_index_arg.raft_index = put_result.index;

    RAFT_CLIENT_PUT_OPTION opt = {0};
    sprintf(opt.callback_woof, "%s/%s", thread_arg->node_addr, DHT_SERVER_PUBLISH_TRIGGER_WOOF);
    sprintf(opt.extra_woof, "%s", DHT_PARTIAL_TRIGGER_WOOF);
    opt.extra_seqno = trigger_seq;
    // log_debug("mapping raft_index %lu to %s", put_result.index, put_result.source);
    unsigned long seq = raft_put_handler(
        put_result.source, "r_map_raft_index", &map_raft_index_arg, sizeof(DHT_MAP_RAFT_INDEX_ARG), 1, &opt);
    if (raft_is_error(seq)) {
        log_error("failed to map data to raft: %s", raft_error_msg);
        return;
    }
    // log_debug("mapped data to raft_index %lu to client_put request %lu", put_result.index, seq);

    return;
}

int server_publish_map(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;

    log_set_tag("server_publish_map");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    uint64_t begin = get_milliseconds();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_SERVER_PUBLISH_MAP_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_SERVER_PUBLISH_MAP_WOOF);
        exit(1);
    }
    int count = latest_seq - routine_arg->last_seqno;
    if (count != 0) {
        log_debug("processing %d publish_map", count);
    }

    zsys_init();
    RESOLVE_THREAD_ARG thread_arg[count];
    pthread_t thread_id[count];

    int i;
    for (i = 0; i < count; ++i) {
        memcpy(thread_arg[i].node_addr, node.addr, sizeof(thread_arg[i].node_addr));
        thread_arg[i].seq_no = routine_arg->last_seqno + 1 + i;
        if (pthread_create(&thread_id[i], NULL, resolve_thread, (void*)&thread_arg[i]) < 0) {
            log_error("failed to create resolve_thread to process publish_map");
            exit(1);
        }
    }
    threads_join(count, thread_id);

    if (count != 0) {
        if (get_milliseconds() - begin > 200)
            log_debug("took %lu ms to process %lu publish_map", get_milliseconds() - begin, count);
    }
    routine_arg->last_seqno = latest_seq;
    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_map", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next server_publish_map");
        exit(1);
    }
    // printf("handler server_publish_map took %lu\n", get_milliseconds() - begin);
    return 1;
}