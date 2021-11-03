#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:"
char* Usage = "test_append_handler -t timeout\n";

typedef struct test_stc {
    char msg[256];
} TEST_EL;

int main(int argc, char** argv) {
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            timeout = (int)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    WooFInit();

    TEST_EL test_el = {0};
    sprintf(test_el.msg, "test_append_handler");
    unsigned long seq = raft_put_handler(NULL, "test_raft_handler", &test_el, sizeof(TEST_EL), NULL);
    if (raft_is_error(seq)) {
        fprintf(stderr, "failed to put %s: %s\n", test_el.msg, raft_error_msg);
    } else {
        printf("put data: %s, index: %lu\n", test_el.msg, seq);
    }

    return 0;
}
