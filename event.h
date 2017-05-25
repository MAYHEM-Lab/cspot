#ifndef EVENT_H
#define EVENT_H

struct event_stc
{
	unsigned char type;
	unsigned long host;
	unsigned long long seq_no;
	unsigned long reason_host;
	unsigned long long reason_seq_no;
};

typedef struct event_stc EVENT;

#define FUNC (1)
#define TRIGGER (2)

EVENT *EventCreate(unsigned char type, unsigned long host,
                   unsigned long long seq_no);

void EventFree(EVENT *ev);

int EventSetReason(EVENT *ev, unsigned long reason_host, 
                                unsigned long long reason_seq_no);


endif

