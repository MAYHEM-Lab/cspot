#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int d_interrupt(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("d_interrupt");
    log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    RAFT_DEBUG_INTERRUPT_ARG arg = {0};
    if (monitor_cast(ptr, &arg) < 0) {
        log_error("failed to monitor_cast");
        exit(1);
    }

    usleep(arg.microsecond);

    monitor_exit(ptr);
    return 1;
}
