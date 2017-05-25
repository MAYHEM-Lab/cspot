#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <pthread.h>
#include "event.h"
#include "mio.h"
#include "log.h"

LOG *LogCreate(char *filename, unsigned long int size)
{
	LOG *log;
	unsigned long int space;
	MIO *mio;

	space = (size+1)*sizeof(EVENT) + sizeof(LOG);

	mio = MIOOpen(filename,"rw",space);
	if(mio == NULL) {
		return(NULL);
	}

	log = (LOG *)MIOAddr(mio);
	log->m_buf = mio;
	memset(log,0,sizeof(LOG));
	strcpy(log->filename,filename);

	log->size = size+1;
	log->seq_no = 1;
	pthread_mutex_init(&log->lock,NULL);

	return(log);
}

void LogFree(LOG *log) 
{
	if(log == NULL) {
		return;
	}


	if(log->m_buf != NULL) {
		MIOClose(log->m_buf);
	}

	return;
}

int LogFull(LOG *log)
{
	unsigned long int next;

	if(log == NULL) {
		return(-1);
	}

	next = (log->head + 1) % log->size;
	if(next == log->tail) {
		return(1);
	} else {
		return(0);
	}
}

unsigned long long LogEvent(LOG *log, EVENT *event)
{
	EVENT *ev_array;
	unsigned long int next;
	unsigned long long seq_no;

	if(log == NULL) {
		return(0);
	}

	if(log->filename == NULL) {
		return(0);
	}

	ev_array = (EVENT *)(MIOAddr(log->m_buf) + sizeof(LOG));

	pthread_mutex_lock(&log->lock);
	if(LogFull(log)) {
		pthread_mutex_unlock(&log->lock);
		return(0);
	}

	next = (log->head + 1) % log->size;
	
	memcpy(&ev_array[next],event,sizeof(EVENT));
	log->head = next;
	log->seq_no++;
	seq_no = log->seq_no;
	pthread_mutex_unlock(&log->lock);

	return(seq_no);
}

PENDING *PendingCreate(char *filename, unsigned long psize)
{
	MIO *mio;
	unsigned long space;
	PENDING *pending;
	EVENT *ev_array;

	space = psize*sizeof(EVENT) + sizeof(PENDING);
	mio = MIOOpen(filename,"rw",space);
	if(mio == NULL) {
		return(NULL);
	}

	pending = (PENDING *)MIOAddr(mio);
	memset(pending,0,sizeof(PENDING));
	pending->alive = RBInitD();
	if(pending->alive == NULL) {
		MIOClose(mio);
		return(NULL);
	}
	pending->p_mio = mio;
	strcpy(pending->filename,filename);
	pending->size = psize;
	pending->next_free = 0;
	ev_array = (EVENT *)(MIOAddr(mio) + sizeof(PENDING));
	memset(ev_array,0,psize*sizeof(EVENT));

	return(pending);
}

void PendingFree(PENDING *pending)
{

	if(pending == NULL) {
		return;
	}

	if(pending->alive != 0) {
		RBDestroyD(pending->alive);
	}

	if(pending->p_mio != NULL) {
		MIOClose(pending->p_mio);
	}

	return;
}


