#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:c:"
char* Usage = "test_get -t topic -c count\n";

typedef struct test_stc {
    char msg[256 - 8];
    uint64_t sent;
} TEST_EL;

int main(int argc, char** argv) {
    char topic_name[DHT_NAME_LENGTH] = {0};
    int count = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic_name, optarg, sizeof(topic_name));
            break;
        }
        case 'c': {
            count = (int)strtoul(optarg, NULL, 10);
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (topic_name[0] == 0 || count == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    int i;
    RAFT_DATA_TYPE data = {0};
    uint64_t sum = 0;
    for (i = 0; i < count; ++i) {
        uint64_t begin = get_milliseconds();
        int err = dht_get(topic_name, &data, 1);
        if (err < 0) {
            fprintf(stderr, "failed to get data from topic: %s\n", dht_error_msg);
            exit(1);
        }
        sum += get_milliseconds() - begin;
    }
    printf("dht_get %d data took %lu ms, avg %f ms", count, sum, (double)sum / count);
    fflush(stdout);

    return 0;
}
