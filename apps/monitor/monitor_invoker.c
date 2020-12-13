#include "monitor.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int monitor_invoker(WOOF* wf, unsigned long seq_no, void* ptr) {
    MONITOR_INVOKER_ARG* arg = (MONITOR_INVOKER_ARG*)ptr;
    unsigned long last_done = WooFGetLatestSeqno(arg->done_woof);
    if (WooFInvalid(last_done)) {
        fprintf(stderr, "can't get the last done seqno from %s\n", arg->done_woof);
        exit(1);
    }
    unsigned long last_queued = WooFGetLatestSeqno(arg->pool_woof);
    if (WooFInvalid(last_queued)) {
        fprintf(stderr, "can't get the last queued seqno from %s\n", arg->pool_woof);
        exit(1);
    }
    if (last_done + 1 <= last_queued) {
        int count = last_queued - last_done;
        MONITOR_POOL_ITEM pool_item[count];
        memset(pool_item, 0, sizeof(MONITOR_POOL_ITEM) * count);
        int i;
        for (i = 0; i < count; ++i) {
            if (WooFGet(arg->pool_woof, &pool_item[i], last_done + 1 + i) < 0) {
                fprintf(stderr,
                        "failed to get queued handler information from %s(%lu)\n",
                        arg->pool_woof,
                        last_done + 1 + i);
                fprintf(stderr, "last_done: %lu, count: %d, i: %d\n", last_done, count, i);

                fprintf(stderr, "queued handlers: %lu\n", count);
                int j;
                for (j = 0; j < i; ++j) {
                    fprintf(stderr, "%s ", pool_item[j].handler);
                }
                printf("\n");
                exit(1);
            }
        }

        if (MONITOR_WARNING_QUEUED_HANDLERS > 0 && count >= MONITOR_WARNING_QUEUED_HANDLERS) {
            if (last_queued % 10 == 0) {
                printf("%s queued handlers: %lu\n", pool_item->monitor_name, count);
            }
        }

        for (i = 0; i < count; ++i) {
            int skip = 0;
            if (pool_item[i].idempotent) {
                int j;
                for (j = i + 1; j < count; ++j) {
                    if ((strcmp(pool_item[i].handler, pool_item[j].handler) == 0) && pool_item[j].idempotent) {
                        skip = 1;
                        break;
                    }
                }
            }
            if (skip) {
                MONITOR_DONE_ITEM done_item;
                unsigned long seq = WooFPut(arg->done_woof, NULL, &done_item);
                if (WooFInvalid(seq)) {
                    fprintf(stderr, "failed to skip handler %s\n", pool_item[i].handler);
                    exit(1);
                }
            } else {
                unsigned long seq = WooFPut(arg->handler_woof, pool_item[i].handler, &pool_item[i]);
                if (WooFInvalid(seq)) {
                    fprintf(stderr, "failed to invoke handler %s\n", pool_item[i].handler);
                    exit(1);
                }
                // if (strcmp(pool_item[i].monitor_name, "raft") == 0) {
                //     printf("invoked %s\n", pool_item[i].handler);
                // }
#ifdef PROCESS_TIME
                struct timeval tv;
                gettimeofday(&tv, NULL);
                printf("took %" PRIu64 "ms to invoke %s\n",
                       ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000) - pool_item[next].queued_ts,
                       pool_item[next].handler);
#endif
                return 0;
            }
        }
    } else {
        usleep(arg->spinlock_delay * 1000);
        ++arg->wasted_cycle;
        unsigned long seq = WooFPut(wf->shared->filename, "monitor_invoker", arg);
        if (WooFInvalid(seq)) {
            fprintf(stderr, "failed to spinlock on %s\n", wf->shared->filename);
            exit(1);
        }
    }

    return 0;
}
