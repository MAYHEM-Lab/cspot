#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#include "mio.h"
#include "event.h"
#include "host.h"
#include "redblack.h"

#include <pthread.h>
#include "lsema.h"

struct log_stc
{
	sema mutex;
	unsigned long host_id;
	unsigned long long seq_no;
	MIO *m_buf;
	char filename[4096];
	unsigned long int size;
	unsigned long int head;
	unsigned long int tail;
};

typedef struct log_stc LOG;

struct pending_stc
{
	MIO *p_mio;
	char filename[4096];
	RB *alive;
	RB *causes;
	unsigned long int size;
	unsigned long next_free;
};

typedef struct pending_stc PENDING;

struct global_log_stc
{
	unsigned long host_id;
	sema mutex;
	char filename[4096];
	LOG *log;
	HOSTLIST *host_list;
	PENDING *pending;
};

typedef struct global_log_stc GLOG;
	

LOG *LogCreate(char *filename, unsigned long host_id, unsigned long int size);
void LogFree(LOG *log);
int LogFull(LOG *log);
unsigned long long LogEvent(LOG *log, EVENT *event);
LOG *LogTail(LOG *log, unsigned long long earliest, unsigned long max_size);
int LogEventEqual(LOG *l1, LOG *l2, unsigned long ndx);
void LogPrint(FILE *fd, LOG *log);

PENDING *PendingCreate(char *filename, unsigned long psize);
void PendingFree(PENDING *pending);
EVENT *PendingFindEvent(PENDING *pending, 
	unsigned long host, unsigned long long seq_no);
EVENT *PendingFindCause(PENDING *pending, 
	unsigned long host, unsigned long long seq_no);
void PendingPrint(FILE *fd, PENDING *pending);


GLOG *GLogCreate(char *filename, unsigned long host_id, unsigned long size);
int GLogEvent(GLOG *gl, EVENT *event);
void GLogFree(GLOG *gl);
void GLogPrint(FILE *fd, GLOG *gl);

int ImportLogTail(GLOG *gl, LOG *ll);
	

#endif
