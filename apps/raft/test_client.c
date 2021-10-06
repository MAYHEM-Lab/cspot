#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "c:t:"
char* Usage = "test_client -c count -t timeout\n";

int main(int argc, char** argv) {
    int count = 10;
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'c': {
            count = (int)strtoul(optarg, NULL, 0);
            break;
        }
        case 't': {
            timeout = (int)strtoul(optarg, NULL, 0);
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

    unsigned long seq[count];
    int i;
    for (i = 0; i < count; ++i) {
        RAFT_DATA_TYPE data = {0};
        sprintf(data.val, "msg_%d", i + 1);
        seq[i] = raft_put(NULL, &data, NULL);
        if (raft_is_error(seq[i])) {
            fprintf(stderr, "failed to put %s: %s\n", data.val, raft_error_msg);
            fflush(stderr);
        } else {
            printf("put data[%d]: %s, client_request_seqno: %lu\n", i, data.val, seq[i]);
            fflush(stdout);
        }
    }
    uint64_t finish = get_milliseconds();
    printf("took %lu ms to async_put %d data\n", finish - begin, count);

    return 0;
}
