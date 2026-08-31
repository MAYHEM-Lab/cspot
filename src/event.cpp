extern "C" {
#include "./include/event.h"
}

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <new>

extern "C" {
EVENT* EventCreate(EventState type, unsigned long host) {
    auto ev = new (std::nothrow) EVENT{};
    if (ev == nullptr) {
        return nullptr;
    }

    ev->type = type;
    ev->host = host;

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ev->timestamp = (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);

    return (ev);
}

void EventFree(EVENT* ev) {
    delete ev;
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
    return ((int64_t)host << 32) + (int64_t)seq_no;
}
}
