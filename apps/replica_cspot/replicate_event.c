#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "woofc.h"
#include "woofc-access.h"
#include "replica.h"

extern LOG *Name_log;
extern char WooF_namespace[2048];
extern char Host_ip[25];

int replicate_event(WOOF *wf, unsigned long seq_no, void *ptr)
{
	REPLICA_EL *el = (REPLICA_EL *)ptr;
	unsigned long curr;
	unsigned long seq;
	EVENT *ev_array;
	SLAVE_PROGRESS slave_progress;
	unsigned int port;
	
	ev_array = (EVENT *)(((void *)Name_log) + sizeof(LOG));
	curr = 0;

	printf("master: %lu, slave: %lu\n", ev_array[Name_log->head].seq_no, el->log_seqno);
	fflush(stdout);
	if (ev_array[Name_log->tail].seq_no > el->log_seqno + 1)
	{
		printf("already rolled over\n");
		fflush(stdout);
	}
	if (ev_array[curr].seq_no < el->log_seqno + 1)
	{
		curr += (el->log_seqno + 1 - ev_array[curr].seq_no);
	}
	else if (ev_array[curr].seq_no > el->log_seqno + 1)
	{
		curr += (Name_log->size - (ev_array[curr].seq_no - (el->log_seqno + 1)));
	}

	port = WooFPortHash(WooF_namespace);
	sprintf(slave_progress.master_namespace, "woof://%s:%u%s", Host_ip, port, WooF_namespace);
	slave_progress.number_event = 0;
	while (curr != ((Name_log->head + 1) % Name_log->size))
	{
		slave_progress.log_seqno = ev_array[curr].seq_no;
		if ((ev_array[curr].type == READ || ev_array[curr].type == APPEND || ev_array[curr].type == LATEST_SEQNO)
			&& strcmp(ev_array[curr].woofc_name, "slave_progress_for_replica") != 0) {
			memcpy(&slave_progress.event[slave_progress.number_event], &ev_array[curr], sizeof(EVENT));
			// printf("sending event[%lu], type: %d, woof: %s, seqno: %lu\n", ev_array[curr].seq_no, ev_array[curr].type, ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
			// fflush(stdout);
			slave_progress.number_event++;
			if (slave_progress.number_event == 1000)
			{
				break;
			}
		}
		curr = (curr + 1) % Name_log->size;
	}
	seq = WooFPut(el->events_woof, "replay_event", &slave_progress);
	if (WooFInvalid(seq))
	{
		fprintf(stderr, "failed to put to %s\n", el->events_woof);
		fflush(stderr);
		exit(1);
	}
	printf("sent %d events to slave\n", slave_progress.number_event);
	fflush(stdout);

	return 1;
}
