#include "avg_temp.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    TEMP_EL temp_el = {0};
    temp_el.value = strtod(argv[1], NULL);

    unsigned long index = dht_publish(ROOM_TEMP_TOPIC, &temp_el, sizeof(TEMP_EL));
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s\n", ROOM_TEMP_TOPIC);
        exit(1);
    }

    return 0;
}
