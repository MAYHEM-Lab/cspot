#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "d:c:t:"
char* Usage = "client_put_result -d data (-c count -t timeout)\n";

int main(int argc, char** argv) {
    RAFT_DATA_TYPE data = {0};
    memset(data.val, 0, sizeof(data.val));
    int count = 1;
    uint64_t timeout = 0;

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
        printf("client_put request seqno: %lu\n", seq);
        RAFT_CLIENT_PUT_RESULT result = {0};
        while (1) {
            if (timeout != 0 && get_milliseconds() - begin > timeout) {
                fprintf(stderr, "timeout after %lu ms\n", timeout);
                exit(1);
            }
            int err = raft_client_put_result(NULL, &result, seq);
            if (err == RAFT_WAITING_RESULT) {
                usleep(DHT_CLIENT_RESULT_DELAY * 1000);
            } else if (err < 0) {
                fprintf(stderr, "failed to get client_put result at %lu: %s\n", seq, raft_error_msg);
                exit(1);
            } else {
                break;
            }
        }
        while (result.redirected_target[0] != 0) {
            printf("request redirected to %s[%lu]\n", result.redirected_target, result.redirected_seqno);
            while (1) {
                if (timeout != 0 && get_milliseconds() - begin > timeout) {
                    fprintf(stderr, "timeout after %lu ms\n", timeout);
                    exit(1);
                }
                int err = raft_client_put_result(result.redirected_target, &result, result.redirected_seqno);
                if (err == RAFT_WAITING_RESULT) {
                    usleep(DHT_CLIENT_RESULT_DELAY * 1000);
                } else if (err < 0) {
                    fprintf(stderr,
                            "failed to get client_put result at %lu from redirected_target %s: %s\n",
                            result.redirected_seqno,
                            result.redirected_target,
                            raft_error_msg);
                    exit(1);
                } else {
                    break;
                }
            }
        }
        printf("data put to raft, index: %lu, term: %lu\n", result.index, result.term);
    }
    
    printf("%d requests took %lu ms", count, get_milliseconds() - begin);
    return 0;
}
