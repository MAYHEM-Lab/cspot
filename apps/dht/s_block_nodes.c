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
    int i;
    for (i = 1; i < argc; ++i) {
        strcpy(blocked_nodes.blocked_nodes[i - 1], argv[i]);
        printf("%s\n", blocked_nodes.blocked_nodes[i - 1]);
    }
    unsigned long seq = WooFPut(BLOCKED_NODES_WOOF, NULL, &blocked_nodes);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to block nodes\n");
        exit(1);
    }
    printf("%d nodes blocked at %" PRIu64 " (%lu)\n", argc - 1, get_milliseconds(), seq);
    return 0;
}
