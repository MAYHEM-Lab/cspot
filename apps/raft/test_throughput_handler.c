#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"
#include "woofc-host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "c:"
char* Usage = "test_throughput_handler (-c count)\n";

typedef struct test_stc {
    char msg[256];
} TEST_EL;

int main(int argc, char** argv) {
    TEST_EL test_el = {0};
    int count = 1;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'c': {
            count = (int)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "Unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    WooFInit();
    uint64_t begin = get_milliseconds();

    int i;
    for (i = 0; i < count; ++i) {
        sprintf(test_el.msg, "msg_%d", i + 1);
        unsigned long seq = raft_put_handler(NULL, "test_raft_handler", &test_el, sizeof(TEST_EL), 0, NULL);
        if (raft_is_error(seq)) {
            fprintf(stderr, "failed to send client_put request: %s\n", raft_error_msg);
            exit(1);
        }
    }

    while (1) {
        RAFT_SERVER_STATE server_state = {0};
        if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
            fprintf(stderr, "failed to get the latest server state\n");
            exit(1);
        }
        if (server_state.commit_index >= count) {
            break;
        }
        usleep(10 * 1000);
    }
    printf("%lu ms to commit %d entries", get_milliseconds() - begin, count);
    return 0;
}
