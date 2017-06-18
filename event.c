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
	if(ev == NULL) {
		return(NULL);
	}
	memset(ev,0,sizeof(EVENT));

	ev->type = type;
	ev->host = host;

	return(ev);
}

void EventFree(EVENT *ev)
{
	free(ev);
	return;
}

int EventSetCause(EVENT *ev, unsigned long cause_host, 
				unsigned long long cause_seq_no)
{
	if(ev == NULL) {
		return(-1);
	}
	ev->cause_host = cause_host;
	ev->cause_seq_no = cause_seq_no;
	return(1);
}

double EventIndex(unsigned long host, unsigned long long seq_no)
{
	double ndx;

	ndx = (double)(((double)host * pow(2,32)) + (double)seq_no);

	return(ndx);
}
