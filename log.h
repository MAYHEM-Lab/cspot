#ifndef LOG_H
#define LOG_H

#include "event.h"
#include "mio.h"
#include "host.h"

#include <pthread.h>

struct log_stc
{
	pthread_mutex_t lock;
	unsigned long long seq_no;
	MIO *m_buf;
	char filename[4096];
	unsigned long int size;
	unsigned long int head;
	unsigned long int tail;
};

typedef log_stc LOG;

struct pending_stc
{
	MIO *p_mio;
	char filename[4096];
	RB *alive;
	RB *reasons;
	unsigned long int size;
	unsigned long next_free;
};

typedef struct pending_stc PENDING;

struct global_log_stc
{
	pthread_mutex_t lock;
	char filename[4096];
	LOG *log;
	HOSTLIST *host_list;
	PENDING *pending;
};

typedef struct global_log_stc GLOG;
	

LOG *LogCreate(char *filename, unsigned long int size);
void LogFree(LOG *log);
int LogFull(LOG *log);
unsigned long long LogEvent(LOG *log, EVENT *event);

PENDING *PendingCreate(char *filename, unsigned long psize);
void PendingFree(PENDING *pending);


PENDING *PendingCreate(char *filename, unsigned long psize);
void GLogFree(GLOG *gl);

	

#endif
