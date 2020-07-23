#include "dht_client.h"
#include "dht_utils.h"

#include <stdio.h>

typedef struct test_stc {
    char msg[256 - 8];
    unsigned long sent;
} TEST_EL;

int test_dht_handler(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    TEST_EL* el = (TEST_EL*)ptr;
    printf("from %s[%lu], sent at %lu:\n", topic_name, seq_no, el->sent);
    printf("%s\n", el->msg);
    printf("took %lu ms\n", get_milliseconds() - el->sent);

#ifdef USE_RAFT
    printf("previous data:\n");

    unsigned long latest_topic_seqno = dht_remote_topic_latest_seqno(woof_name, topic_name);
    if (WooFInvalid(latest_topic_seqno)) {
        fprintf(stderr, "failed to get latest seqno of %s from %s\n", topic_name, woof_name);
        exit(1);
    }
    unsigned long i;
    for (i = latest_topic_seqno; i != 0; --i) {
        TEST_EL prev = {0};
        unsigned long one_min_earlier = get_milliseconds() - (60 * 1000);
        int err = dht_remote_topic_get_range(woof_name, topic_name, &prev, sizeof(prev), i, one_min_earlier, 0);
        if (err == -2) {
            break;
        } else if (err == -1) {
            fprintf(stderr, "failed to get topic data[%d]\n", i);
            exit(1);
        }
        printf("[%lu]: %s\n", i, prev.msg);
    }
#endif
    return 1;
}