#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"
#include "woofc-host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "i:"
char* Usage = "client_get -i index\n";

int main(int argc, char** argv) {
    unsigned long index = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'i': {
            index = strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "Unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (index == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    RAFT_DATA_TYPE data = {0};
    int err = raft_get(NULL, &data, index);
    if (err < 0) {
        fprintf(stderr, "failed to get data: %s\n", raft_error_msg);
        exit(1);
    }
    printf("%s\n", data.val);

    return 0;
}
