#include "dht_client.h"
#include "dht_utils.h"

#include <inttypes.h>
#include <stdio.h>

typedef struct test_stc {
    char msg[256 - 8];
    uint64_t sent;
} TEST_EL;

int test_dht_handler(char* topic_name, unsigned long index, void* ptr) {
    TEST_EL* el = (TEST_EL*)ptr;
    printf("%s triggered at %" PRIu64 "\n", el->msg, get_milliseconds());
    // printf("latest_index: %lu, last 5: ", index);
    // int i;
    // RAFT_DATA_TYPE data;
    // for (i = 0; i < 5 && index - i > 0; ++i) {
    //     if (dht_get(topic_name, &data, index - i) < 0) {
    //         fprintf(stderr, "failed to get topic data from %s[%lu]", topic_name, index - i);
    //     }
    //     el = (TEST_EL*)&data;
    //     printf("%s ", el->msg);
    // }
    // printf("\n");

    return 1;
}