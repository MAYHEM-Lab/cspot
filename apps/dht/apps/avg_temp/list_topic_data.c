#include "avg_temp.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    unsigned long seqno = dht_local_topic_latest_seqno(argv[1]);
    if (WooFInvalid(seqno)) {
        fprintf(stderr, "failed to get latest seqno of %s from %s\n", argv[1]);
        exit(1);
    }

    unsigned long i;
    for (i = seqno; i != 0; --i) {
        TEMP_EL temp_el = {0};
        int err = dht_local_topic_get_range(argv[1], &temp_el, sizeof(TEMP_EL), i, 0, 0);
        if (err < 0) {
            fprintf(stderr, "failed to get topic data[%lu]\n", i);
            exit(1);
        }
        printf("[%lu]: %f\n", i, temp_el.value);
    }

    return 0;
}
