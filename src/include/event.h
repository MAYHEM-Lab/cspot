#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum EventState
{
    UNKNOWN = 1,
    FUNC = 2,
    TRIGGER = 3,
    TRIGGER_FIRING = 4,
    FIRED = 5,
    APPEND = 6,
    READ = 7,
    LATEST_SEQNO = 8,
} EventState;

struct event_stc {
    EventState type;
    unsigned long host;
    unsigned long long seq_no;
    unsigned long cause_host;
    unsigned long long cause_seq_no;
    /*
     * woofc payload
     */
    unsigned long woofc_seq_no;
    unsigned long woofc_ndx;
    unsigned long woofc_element_size;
    unsigned long woofc_history_size;
    char woofc_name[128];
    char woofc_handler[128];
    char woofc_namespace[1024];
    unsigned long ino; // for caching if enabled
    uint64_t timestamp;
// handler tracking
#ifdef TRACK
    int hid;
#endif
};

typedef struct event_stc EVENT;

#ifdef REPAIR
#define MARKED (32) // for downstream events discovery in repair mode
#define ROOT (64)
#define INVALID (128)
#endif

EVENT* EventCreate(EventState type, unsigned long host);

void EventFree(EVENT* ev);

int EventSetCause(EVENT* ev, unsigned long cause_host, unsigned long long cause_seq_no);

int64_t EventIndex(unsigned long host, unsigned long long seq_no);

#if defined(__cplusplus)
}

extern "C++" {
#include <memory>

namespace cspot {
struct event_deleter {
    void operator()(EVENT* ev) {
        EventFree(ev);
    }
};

using event_ptr = std::unique_ptr<EVENT, event_deleter>;

inline event_ptr make_event(EventState type, unsigned long host) {
    return event_ptr(EventCreate(type, host));
}
} // namespace cspot
}
#endif

#endif
