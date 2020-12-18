#include "event.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

EVENT* EventCreate(unsigned char type, unsigned long host) {
    EVENT* ev;
    struct timespec ts;

    ev = (EVENT*)malloc(sizeof(EVENT));
    if (ev == NULL) {
        return (NULL);
    }
    memset(ev, 0, sizeof(EVENT));

    ev->type = type;
    ev->host = host;

    clock_gettime(CLOCK_REALTIME, &ts);
    ev->timestamp = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    return (ev);
}

void EventFree(EVENT* ev) {
    free(ev);
    return;
}

int EventSetCause(EVENT* ev, unsigned long cause_host, unsigned long long cause_seq_no) {
    if (ev == NULL) {
        return (-1);
    }
    ev->cause_host = cause_host;
    ev->cause_seq_no = cause_seq_no;
    return (0);
}

int64_t EventIndex(unsigned long host, unsigned long long seq_no) {
    int64_t ndx;

    ndx = ((int64_t)host << 32) + (int64_t)seq_no;

    return (ndx);
}
