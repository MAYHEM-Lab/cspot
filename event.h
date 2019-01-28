#ifndef EVENT_H
#define EVENT_H

#define REPAIR

#include <stdlib.h>

struct event_stc
{
	unsigned char type;
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
	char namespace[1024];
	unsigned long ino; // for caching if enabled
};

typedef struct event_stc EVENT;

#define UNKNOWN (1)
#define FUNC (2)
#define TRIGGER (3)
#define TRIGGER_FIRING (4)
#define FIRED (5)
#define APPEND (6)
#define READ (7)
#define LATEST_SEQNO (8)

#ifdef REPAIR
#define MARKED (32) // for downstream events discovery in repair mode
#define INVALID (64)
#endif

EVENT *EventCreate(unsigned char type, unsigned long host);

void EventFree(EVENT *ev);

int EventSetCause(EVENT *ev, unsigned long cause_host,
				  unsigned long long cause_seq_no);

int64_t EventIndex(unsigned long host, unsigned long long seq_no);

#endif
