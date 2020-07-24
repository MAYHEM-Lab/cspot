#include "monitor.h"
#include "raft.h"
#include "woofc-host.h"

#include <stdlib.h>

int main(int argc, char** argv) {
    RAFT_DEBUG_INTERRUPT_ARG arg = {0};
    arg.microsecond = strtoul(argv[1], NULL, 10) * 1000;

    WooFInit();

    unsigned long seq = monitor_put(RAFT_MONITOR_NAME, RAFT_DEBUG_INTERRUPT_WOOF, "d_interrupt", &arg, 0);
    if (WooFInvalid(seq)) {
        log_error("failed to put d_interrupt handler");
        exit(1);
    }

    return 0;
}
