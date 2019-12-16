#ifndef replica_H
#define replica_H

#define EVENT_PER_BATCH 256
#define PROGRESS_WOOF_NAME "progress_for_replica"
#define EVENTS_WOOF_NAME "events_woof_for_replica"

struct slave_progress
{
	char events_woof[256];
	unsigned long log_seqno;
};

typedef struct slave_progress SLAVE_PROGRESS;

struct event_batch
{
	unsigned long log_seqno;
	char master_namespace[256];
	int number_event;
	EVENT event[EVENT_PER_BATCH];
};

typedef struct event_batch EVENT_BATCH;

#endif