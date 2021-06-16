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
char* Usage = "client_put -d data (-c count)\n";

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
    printf("%d requests took %lu ms", count, get_milliseconds() - begin);
    return 0;
}
