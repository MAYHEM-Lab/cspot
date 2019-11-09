#ifndef replica_H
#define replica_H

#define EVENT_PER_BATCH 256

struct replica_stc
{
	char events_woof[256];
	unsigned long log_seqno;
	// unsigned long events_count;
};

typedef struct replica_stc REPLICA_EL;

struct slave_progress
{
	unsigned long log_seqno;
	char master_namespace[256];
	int number_event;
	EVENT event[EVENT_PER_BATCH];
};

typedef struct slave_progress SLAVE_PROGRESS;

#endif