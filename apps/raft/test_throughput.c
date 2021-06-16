#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"
#include "woofc-host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "d:c:"
char* Usage = "test_throughput -d data (-c count)\n";

int main(int argc, char** argv) {
    RAFT_DATA_TYPE data = {0};
    memset(data.val, 0, sizeof(data.val));
    int count = 1;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'd': {
            strncpy(data.val, optarg, sizeof(data.val));
            break;
        }
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

    if (data.val[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();
    uint64_t begin = get_milliseconds();

    int i;
    for (i = 0; i < count; ++i) {
        unsigned long seq = raft_put(NULL, &data, NULL);
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
