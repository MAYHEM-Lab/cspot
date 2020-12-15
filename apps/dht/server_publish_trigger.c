#include "dht.h"
#include "dht_utils.h"
#include "raft_client.h"
#include "string.h"
#include "woofc.h"

#define MAX_PUBLISH_SIZE 64

typedef struct resolve_thread_arg {
    unsigned long seq_no;
} RESOLVE_THREAD_ARG;

void* resolve_thread(void* arg) {
    RESOLVE_THREAD_ARG* thread_arg = (RESOLVE_THREAD_ARG*)arg;
    RAFT_CLIENT_PUT_RESULT put_result = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_TRIGGER_WOOF, &put_result, thread_arg->seq_no) < 0) {
        log_error("failed to get put_result at %lu", thread_arg->seq_no);
        return;
    }

    DHT_TRIGGER_ARG trigger_arg = {0};
    if (WooFGet(put_result.extra_woof, &trigger_arg, put_result.extra_seqno) < 0) {
        log_error("failed to get trigger_arg from %s at %lu", put_result.extra_woof, put_result.extra_seqno);
        return;
    }
    trigger_arg.data = get_milliseconds();
    trigger_arg.element_seqno = put_result.index;

    unsigned long seq = WooFPut(DHT_TRIGGER_WOOF, NULL, &trigger_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue trigger to %s", DHT_TRIGGER_WOOF);
        return;
    }
    return;
}

int server_publish_trigger(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;
    log_set_tag("server_publish_trigger");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    zsys_init();

    uint64_t begin = get_milliseconds();

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_SERVER_PUBLISH_TRIGGER_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_SERVER_PUBLISH_TRIGGER_WOOF);
        exit(1);
    }
    int count = latest_seq - routine_arg->last_seqno;
    if (count > MAX_PUBLISH_SIZE) {
        log_warn("publish_trigger lag: %d", count);
        count = MAX_PUBLISH_SIZE;
    }
    if (count != 0) {
        log_debug("processing %d publish_trigger", count);
    }

    RESOLVE_THREAD_ARG thread_arg[count];
    pthread_t thread_id[count];

    int i;
    for (i = 0; i < count; ++i) {
        thread_arg[i].seq_no = routine_arg->last_seqno + 1 + i;
        if (pthread_create(&thread_id[i], NULL, resolve_thread, (void*)&thread_arg[i]) < 0) {
            log_error("failed to create resolve_thread to process publish_trigger");
            exit(1);
        }
    }
    routine_arg->last_seqno += count;

    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_trigger", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next server_publish_trigger");
        exit(1);
    }

    threads_join(count, thread_id);
    if (count != 0) {
        if (get_milliseconds() - begin > 200)
            log_debug("took %lu ms to process %lu publish_trigger", get_milliseconds() - begin, count);
    }
    // printf("handler server_publish_trigger took %lu\n", get_milliseconds() - begin);
    return 1;
}