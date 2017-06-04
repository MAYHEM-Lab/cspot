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

#define UNKNOWN (1)
#define FUNC (2)
#define TRIGGER (3)

EVENT *EventCreate(unsigned char type, unsigned long host);

void EventFree(EVENT *ev);

int EventSetReason(EVENT *ev, unsigned long reason_host, 
                                unsigned long long reason_seq_no);

double EventIndex(unsigned long host, unsigned long long seq_no);


#endif

