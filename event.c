#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "event.h"

EVENT *EventCreate(unsigned char type, unsigned long host, 
		   unsigned long long seq_no)
{
	EVENT *ev;

	ev = (EVENT *)malloc(sizeof(EVENT));
	if(ev == NULL) {
		return(NULL);
	}
	memset(ev,0,sizeof(EVENT));

	ev->type = type;
	ev->host = host;
	ev->seq_no = seq_no;

	return(ev);
}

void EventFree(EVENT *ev)
{
	free(ev);
	return;
}

int EventSetReason(EVENT *ev, unsigned long reason_host, 
				unsigned long long reason_seq_no)
{
	if(ev == NULL) {
		return(-1);
	}
	ev->reason_host = reason_host;
	ev->reason_seq_no = reason_seq_no;
	return(1);
}

double EventIndex(unsigned long host, unsigned long long seq_no)
{
	double ndx;

	ndx = (double)(((double)host * pow(2,64)) + (double)seq_no);

	return(ndx);
}
