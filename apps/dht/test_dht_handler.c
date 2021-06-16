#include "dht_client.h"
#include "dht_utils.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct test_stc {
    char msg[256 - 8];
    uint64_t sent;
} TEST_EL;

int test_dht_handler(char* topic_name, unsigned long seq_no, void* ptr) {
    TEST_EL* el = (TEST_EL*)ptr;
    printf("%s triggered at %" PRIu64 "\n", el->msg, get_milliseconds());
    // char remaining[256] = {0};
    // unsigned long index = dht_latest_earlier_index(topic_name, seq_no);
    // sprintf(remaining, "last 5: ");
    // int i;
    // RAFT_DATA_TYPE data;
    // for (i = 0; i < 5 && index - i > 0; ++i) {
    //     if (dht_get(topic_name, &data, index - i) < 0) {
    //         fprintf(stderr, "failed to get topic data from %s[%lu]", topic_name, index - i);
    //     }
    //     el = (TEST_EL*)&data;
    //     sprintf(remaining + strlen(remaining), "%s ", el->msg);
    // }
    // sprintf(remaining + strlen(remaining), "\n");
    // printf(remaining);

    return 1;
}