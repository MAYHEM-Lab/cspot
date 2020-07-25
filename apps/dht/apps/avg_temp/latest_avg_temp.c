#include "avg_temp.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    unsigned long seqno = dht_local_topic_latest_seqno(AVG_TEMP_TOPIC);
    if (WooFInvalid(seqno)) {
        fprintf(stderr, "failed to get latest seqno of %s from %s\n", AVG_TEMP_TOPIC);
        exit(1);
    }

    TEMP_EL temp_el = {0};
    int err = dht_local_topic_get_range(AVG_TEMP_TOPIC, &temp_el, sizeof(TEMP_EL), seqno, 0, 0);
    if (err < 0) {
        fprintf(stderr, "failed to get topic data[%lu]\n", seqno);
        exit(1);
    }
    printf("[%lu]: %f\n", seqno, temp_el.value);

    return 0;
}
