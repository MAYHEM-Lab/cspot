#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "event.h"

EVENT *EventCreate(unsigned char type, unsigned long host)
{
	EVENT *ev;

	ev = (EVENT *)malloc(sizeof(EVENT));
	if (ev == NULL)
	{
		return (NULL);
	}
	memset(ev, 0, sizeof(EVENT));

	ev->type = type;
	ev->host = host;

	return (ev);
}

void EventFree(EVENT *ev)
{
	free(ev);
	return;
}

int EventSetCause(EVENT *ev, unsigned long cause_host,
				  unsigned long long cause_seq_no)
{
	if (ev == NULL)
	{
		return (-1);
	}
	ev->cause_host = cause_host;
	ev->cause_seq_no = cause_seq_no;
	return (1);
}

int64_t EventIndex(unsigned long host, unsigned long long seq_no)
{
	int64_t ndx;

	ndx = ((int64_t)host << 32) + (int64_t)seq_no;

	return (ndx);
}
