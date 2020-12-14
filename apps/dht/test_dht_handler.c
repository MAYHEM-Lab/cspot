#include "dht_utils.h"

#include <inttypes.h>
#include <stdio.h>

typedef struct test_stc {
    char msg[256 - 8];
    uint64_t sent;
} TEST_EL;

int get_element(char* raft_addr, void* element, unsigned long seq_no);

int test_dht_handler(char* raft_addr, char* topic_name, unsigned long seq_no, void* ptr) {
    TEST_EL* el = (TEST_EL*)ptr;
    // printf("from %s[%lu], sent at %lu:\n", topic_name, seq_no, el->sent);
    // printf("%s\n", el->msg);
    // printf("took %lu ms\n", get_milliseconds() - el->sent);
    printf("%s triggered at %" PRIu64 "\n", el->msg, get_milliseconds());
    // printf("last 5: ");
    // int i;
    // for (i = 1; i <= 5 && seq_no - i > 0; ++i) {
    //     if (get_element(raft_addr, el, seq_no - i) < 0) {
    //         printf("\nfailed to get %lu\n", seq_no - i);
    //         break;
    //     }
    //     printf("%s ", el->msg);
    // }
    // printf("\n");

    return 1;
}