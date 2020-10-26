#include "dht.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char** argv) {
    WooFInit();

    BLOCKED_NODES blocked_nodes = {0};
    int i, j;
    for (i = 1, j = 0; i < argc; i += 3, ++j) {
        strcpy(blocked_nodes.blocked_nodes[j], argv[i]);
        blocked_nodes.failure_rate[j] = atoi(argv[i + 1]);
        blocked_nodes.timeout[j] = atoi(argv[i + 2]);
        printf("%s fails with %d%% chance\n", blocked_nodes.blocked_nodes[j], blocked_nodes.failure_rate[j]);
    }
    unsigned long seq = WooFPut(BLOCKED_NODES_WOOF, NULL, &blocked_nodes);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to block nodes\n");
        exit(1);
    }
    printf("%d nodes blocked at %" PRIu64 " (%lu)\n", argc - 1, get_milliseconds(), seq);
    return 0;
}
