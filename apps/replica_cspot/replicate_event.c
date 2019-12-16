#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log.h"
#include "woofc.h"
#include "woofc-access.h"
#include "replica.h"

extern LOG *Name_log;
extern char WooF_namespace[2048];
extern char Host_ip[25];

int replicate_event(WOOF *wf, unsigned long seq_no, void *ptr)
{
	SLAVE_PROGRESS *slave_progress = (SLAVE_PROGRESS *)ptr;
	unsigned long curr;
	unsigned long seq;
	EVENT *ev_array;
	EVENT_BATCH event_batch;
	unsigned int port;
	unsigned long num_read = 0, num_latest = 0, num_append = 0;
	struct timeval t1, t2;
	struct timeval begin, end;
	
	ev_array = (EVENT *)(((void *)Name_log) + sizeof(LOG));
	curr = 0;

	gettimeofday(&begin, NULL);

	printf("master: %lu, slave: %lu\n", ev_array[Name_log->head].seq_no, slave_progress->log_seqno);
	fflush(stdout);
	if (ev_array[Name_log->tail].seq_no > slave_progress->log_seqno + 1)
	{
		fprintf(stderr, "already rolled over\n");
		fflush(stderr);
		return 1;
	}
	if (ev_array[curr].seq_no < slave_progress->log_seqno + 1)
	{
		curr += (slave_progress->log_seqno + 1 - ev_array[curr].seq_no);
	}
	else if (ev_array[curr].seq_no > slave_progress->log_seqno + 1)
	{
		curr += (Name_log->size - (ev_array[curr].seq_no - (slave_progress->log_seqno + 1)));
	}

	port = WooFPortHash(WooF_namespace);
	sprintf(event_batch.master_namespace, "woof://%s:%u%s", Host_ip, port, WooF_namespace);
	event_batch.number_event = 0;
	while (curr != ((Name_log->head + 1) % Name_log->size))
	{
		if (strcmp(ev_array[curr].woofc_name, PROGRESS_WOOF_NAME) != 0)
		{
			event_batch.log_seqno = ev_array[curr].seq_no;
			if (ev_array[curr].type == READ)
			{
				num_read++;
			}
			else if (ev_array[curr].type == LATEST_SEQNO)
			{
				num_latest++;
			}
			else if (ev_array[curr].type == APPEND)
			{
				num_append++;
				memcpy(&event_batch.event[event_batch.number_event], &ev_array[curr], sizeof(EVENT));
				// printf("sending event[%lu], type: %d, woof: %s, seqno: %lu\n", ev_array[curr].seq_no, ev_array[curr].type, ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
				// fflush(stdout);
				event_batch.number_event++;
				if (event_batch.number_event == EVENT_PER_BATCH)
				{
					break;
				}
			}
		}
		curr = (curr + 1) % Name_log->size;
	}
	gettimeofday(&t1, NULL);
	seq = WooFPut(slave_progress->events_woof, "replay_event", &event_batch);
	gettimeofday(&t2, NULL);
	if (WooFInvalid(seq))
	{
		fprintf(stderr, "failed to put to %s\n", slave_progress->events_woof);
		fflush(stderr);
		exit(1);
	}
	
	gettimeofday(&end, NULL);
	printf("total(%d): %lu ms\n", event_batch.number_event, (unsigned long)((end.tv_sec - begin.tv_sec) * 1000.0 + (end.tv_usec - begin.tv_usec) / 1000.0));
	printf("msg(%d): %lu ms\n", event_batch.number_event, (unsigned long)((t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0));
	printf("num_read: %lu, num_latest: %lu, num_append: %lu\n", num_read, num_latest, num_append);
	fflush(stdout);

	return 1;
}
