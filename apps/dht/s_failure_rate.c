#include "dht.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <string.h>


int main(int argc, char** argv) {
    WooFInit();

    if (argc != 2) {
        fprintf(stderr, "./s_failure_rate failure_percentage\n");
        exit(1);
    }
    FAILURE_RATE failure_rate = {0};
    failure_rate.failed_percentage = atoi(argv[1]);

    unsigned long seq = WooFPut(FAILURE_RATE_WOOF, NULL, &failure_rate);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to set failure percentage\n");
        exit(1);
    }
    printf("set failure percentage to %d\%\n", failure_rate.failed_percentage);
    return 0;
}
