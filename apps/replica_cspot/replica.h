#ifndef replica_H
#define replica_H

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
	EVENT event[1000];
};

typedef struct slave_progress SLAVE_PROGRESS;

#endif