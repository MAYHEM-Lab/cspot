#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "event.h"
#include "mio.h"
#include "log.h"



LOG *LogCreate(char *filename, unsigned long host_id, unsigned long int size)
{
	LOG *log;
	unsigned long int space;
	MIO *mio;

	space = (size+1)*sizeof(EVENT) + sizeof(LOG);

	if(filename != NULL) {
		mio = MIOOpen(filename,"w+",space);
	} else {
		mio = MIOMalloc(space);
	}

	if(mio == NULL) {
		return(NULL);
	}


	log = (LOG *)MIOAddr(mio);
	memset(log,0,sizeof(LOG));

	log->m_buf = mio;
	if(filename != NULL) {
		strcpy(log->filename,filename);
	}


	log->host_id = host_id;
	log->size = size+1;
	log->seq_no = 1;
	InitSem(&log->mutex,1);
	/*
	 * for WooF launch synchronization
	 */
	InitSem(&log->tail_wait,0);

	return(log);
}

LOG *LogOpen(char *filename,unsigned long size)
{
	LOG *log;
	unsigned long int space;
	MIO *mio;

	if(filename == NULL) {
		return(NULL);
	}
	space = (size+1)*sizeof(EVENT) + sizeof(LOG);
	mio = MIOOpen(filename,"a+",space);

	if(mio == NULL) {
		return(NULL);
	}


	log = (LOG *)MIOAddr(mio);

	return(log);
}

void LogFree(LOG *log) 
{
	if(log == NULL) {
		return;
	}

	FreeSem(&log->mutex);


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

int LogAdd(LOG *log, EVENT *event)
{
	EVENT *ev_array;
	unsigned long int next;
	unsigned long long seq_no;

	if(log == NULL) {
		return(1);
	}

	ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

	/*
	 * FIX ME: need to implement a cleaner that moves the tail
	 */
	if(LogFull(log)) {
		log->tail = (log->tail + 1) % log->size;
	}

	next = (log->head + 1) % log->size;
	
	memcpy(&ev_array[next],event,sizeof(EVENT));
	log->head = next;

	return(1);
}

int LogEventEqual(LOG *l1, LOG *l2, unsigned long ndx)
{
	EVENT *e1;
	EVENT *e2;

	e1 = (EVENT *)(MIOAddr(l1->m_buf) + sizeof(LOG));
	e2 = (EVENT *)(MIOAddr(l2->m_buf) + sizeof(LOG));

	if(memcmp(&e1[ndx],&e2[ndx],sizeof(EVENT)) != 0) {
		return(0);
	} else {
		return(1);
	}
}


unsigned long long LogEvent(LOG *log, EVENT *event)
{
	int err;
	unsigned long long seq_no;
	MIO *mio;

	if(log == NULL) {
		return(0);
	}


	P(&log->mutex);		/* single thread for now */
	seq_no = log->seq_no;
	event->seq_no = seq_no;
	log->seq_no++;
#ifdef DEBUG
	printf("LogEvent: calling LogAdd\n");
	fflush(stdout);
#endif
	err = LogAdd(log,event);
#ifdef DEBUG
	printf("LogEvent: added\n");
	fflush(stdout);
#endif
	V(&log->mutex);
	
	return(seq_no);
}

/*
 * exactly the same as previous one but without mutex so logging can be done
 * atomically with other functions
 */
unsigned long long LogEventNoLock(LOG *log, EVENT *event)
{
	int err;
	unsigned long long seq_no;
	MIO *mio;

	if(log == NULL) {
		return(0);
	}


	seq_no = log->seq_no;
	event->seq_no = seq_no;
	log->seq_no++;
#ifdef DEBUG
	printf("LogEventNoLock: calling LogAdd\n");
	fflush(stdout);
#endif
	err = LogAdd(log,event);
#ifdef DEBUG
	printf("LogEventNoLock: added\n");
	fflush(stdout);
#endif
	
	return(seq_no);
}


/*
 * not thread safe
 */
LOG *LogTail(LOG *log, unsigned long long earliest, unsigned long max_size)
{
	LOG *log_tail;
	unsigned long curr;
	EVENT *ev_array;
	unsigned long count;

	if(log->head == log->tail) { /* log is empty */
		return(NULL);
	}

	log_tail = LogCreate(NULL,0,max_size);
	if(log_tail == NULL) {
		return(NULL);
	}

//	ev_array = (EVENT *)(MIOAddr(log->m_buf) + sizeof(LOG));
	ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

	/*
	 * put events in the tail log in reverse order to save a scan
	 */
	count = 0;
	curr = log->head;
	while(ev_array[curr].seq_no >= earliest) {
		/*
		 * allow for ealiest to overlap
		 */
		if(ev_array[curr].seq_no >= earliest) {
			LogAdd(log_tail,&ev_array[curr]);
			count++;
			if(count >= max_size) {
				break;
			}
		}
		curr = curr - 1;
		if(curr >= log->size) {
			curr = (log->size-1);
		}
		if(curr == log->tail) { /* this behind the last valid */
			break;
		}
	}

	return(log_tail);
}
		
	
void LogPrint(FILE *fd, LOG *log)
{
	unsigned long curr;
	EVENT *ev_array;

//	ev_array = (EVENT *)(MIOAddr(log->m_buf) + sizeof(LOG));
	ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

	fprintf(fd,"log filename: %s\n",log->filename);
	fprintf(fd,"log size: %lu\n",log->size);
	fprintf(fd,"log head: %lu\n",log->head);
	fprintf(fd,"log tail: %lu\n",log->tail);

	curr = log->head;
	while(curr != log->tail) {
		fprintf(fd,
		"\t[%lu] host: %lu seq_no: %llu r_host: %lu r_seq_no: %llu\n",
			curr,
			ev_array[curr].host,	
			ev_array[curr].seq_no,	
			ev_array[curr].cause_host,	
			ev_array[curr].cause_seq_no);	
		fflush(fd);
		curr = curr - 1;
		if(curr >= log->size) {
			curr = log->size - 1;
		}
	}

	fflush(fd);
	return;
}

PENDING *PendingCreate(char *filename, unsigned long psize)
{
	MIO *mio;
	unsigned long space;
	PENDING *pending;
	EVENT *ev_array;

	space = psize*sizeof(EVENT) + sizeof(PENDING);
	mio = MIOOpen(filename,"w+",space);
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

	pending->causes = RBInitD();
	if(pending->causes == NULL) {
		RBDestroyD(pending->alive);
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

	if(pending->alive != NULL) {
		RBDestroyD(pending->alive);
	}

	if(pending->causes != NULL) {
		RBDestroyD(pending->causes);
	}

	if(pending->p_mio != NULL) {
		MIOClose(pending->p_mio);
	}

	return;
}

int PendingAddEvent(PENDING *pending, EVENT *event) 
{
	double ndx;
	EVENT *local_event;
	unsigned long start;
	EVENT *ev_array;
	int fail;

	/*
	 * first see if it is already there
	 */
	local_event = PendingFindEvent(pending,event->host,event->seq_no);
	if(local_event != NULL) {
		fprintf(stderr,"found %lu %llu pending while adding\n",
			event->host, event->seq_no);
		fflush(stderr);
		return(-1);
	}

	ev_array = (EVENT *)(MIOAddr(pending->p_mio)+sizeof(PENDING));
	start = pending->next_free;
	fail = 0;
	while(ev_array[pending->next_free].seq_no != 0) {
		pending->next_free = (pending->next_free + 1) % pending->size;
		if(pending->next_free == start) {
			fail = 1;
			break;
		}
	}

	if(fail == 1) {
		fprintf(stderr,
			"couldn't find free space for pending %lu %llu\n",
				event->host,
				event->seq_no);
		fflush(stderr);
		return(-1);
	}
	memcpy(&ev_array[pending->next_free],event,sizeof(EVENT));
	ndx = EventIndex(event->host,event->seq_no);
	RBInsertD(pending->alive,ndx,
		  (Hval)(void *)(&ev_array[pending->next_free]));
	ndx = EventIndex(event->cause_host,event->cause_seq_no);
	RBInsertD(pending->causes,ndx,
		(Hval)(void *)(&ev_array[pending->next_free]));

	return(1);
}

int PendingRemoveEvent(PENDING *pending, EVENT *event)
{
	RB *rb;
	RB *start;
	EVENT *local_event;
	EVENT *this_event;
	double ndx;
	int found = 0;

	ndx = EventIndex(event->host,event->seq_no);
	rb = RBFindD(pending->alive,ndx);
	if(rb == NULL) {
		fprintf(stderr,
		"couldn't find %lu %llu pending alive\n",
			event->host,
			event->seq_no);
		fflush(stderr);
		return(-1);
	}
	local_event = (EVENT *)rb->value.v;
	RBDeleteD(pending->alive,rb);

	/*
	 * there could be multiple events with the same cause event
	 *
	 * find the one that matches the #event#
	 */
	ndx = EventIndex(event->cause_host,event->cause_seq_no);

	start = RBFindD(pending->causes,ndx);
	if(start == NULL) {
		fprintf(stderr,
		"couldn't find %lu %llu pending cause\n",
			event->host,
			event->seq_no);
		fflush(stderr);
		return(-1);
	}
	rb = start;
	while((rb != NULL) && (rb->key.key.d == ndx)) {
		if(rb->value.v == local_event) {
			RBDeleteD(pending->causes,rb);
			local_event->seq_no = 0; /* marks MIO space as free */
			return(1);
		}
		rb = rb->next;
	}
	rb = start;
	while((rb != NULL) && (rb->key.key.d == ndx)) {
		if(rb->value.v == local_event) {
			RBDeleteD(pending->causes,rb);
			local_event->seq_no = 0; /* marks MIO space as free */
			return(1);
		}
		rb = rb->prev;
	}

	return(-1);
}


EVENT *PendingFindEvent(PENDING *pending, unsigned long host, unsigned long long seq_no)
{
	double ndx = EventIndex(host,seq_no);
	RB *rb;
	EVENT *ev;

	rb = RBFindD(pending->alive,ndx);
	if(rb == NULL) {
		return(NULL);
	}

	ev = (EVENT *)rb->value.v;
	/*
	 * careful here since we are not making a copy => can't wrap the log
	 * before this gets used
	 */

	/*
	 * fix this => multiple cause records can point to the same
	 * event record
	 */

	return(ev);
}

EVENT *PendingFindCause(PENDING *pending, unsigned long host, unsigned long long seq_no)
{
	double ndx = EventIndex(host,seq_no);
	RB *rb;
	EVENT *ev;

	rb = RBFindD(pending->causes,ndx);
	if(rb == NULL) {
		return(NULL);
	}

	ev = (EVENT *)rb->value.v;
	/*
	 * careful here since we are not making a copy => can't wrap the log
	 * before this gets used
	 */

	return(ev);
}

void PendingPrint(FILE *fd, PENDING *pending)
{
	RB *rb;
	EVENT *ev;

	fprintf(fd,"pending\n");
	RB_FORWARD(pending->alive,rb) {
		ev = (EVENT *)rb->value.v;
		fprintf(fd,
		 "\thost: %lu seq_no: %llu r_host: %lu r_seq_no: %llu\n",
			ev->host,
			ev->seq_no,
			ev->cause_host,
			ev->cause_seq_no);
		fflush(fd);
	}

	return;
}


GLOG *GLogCreate(char *filename, unsigned long host_id, unsigned long size)
{
	GLOG *gl;
	MIO *mio;
	unsigned long space;
	char subfilename[4096];

	space = sizeof(GLOG);

	mio = MIOOpen(filename,"w+",space);
	if(mio == NULL) {
		return(NULL);
	}

	gl = (GLOG *)MIOAddr(mio);
	strcpy(gl->filename,filename);

	memset(subfilename,0,sizeof(subfilename));
	strcpy(subfilename,filename);
	strcat(subfilename,".host");

	gl->host_list = HostListCreate(subfilename);
	if(gl->host_list == NULL) {
		GLogFree(gl);
		return(NULL);
	}

	memset(subfilename,0,sizeof(subfilename));
	strcpy(subfilename,filename);
	strcat(subfilename,".log");
	gl->log = LogCreate(subfilename,0,size);
	if(gl->log == NULL) {
		GLogFree(gl);
		return(NULL);
	}

	memset(subfilename,0,sizeof(subfilename));
	strcpy(subfilename,filename);
	strcat(subfilename,".pending");
	gl->pending = PendingCreate(subfilename,size*5);
	if(gl->pending == NULL) {
		GLogFree(gl);
		return(NULL);
	}

	InitSem(&gl->mutex,1);

	gl->host_id = host_id;

	return(gl);
}

void GLogFree(GLOG *gl)
{
	if(gl->host_list != NULL) {
		HostListFree(gl->host_list);
	}

	FreeSem(&gl->mutex);

	if(gl->log != NULL) {
		LogFree(gl->log);
	}

	if(gl->pending != NULL) {
		PendingFree(gl->pending);
	}

	return;
}

/*
 * terminate the dependency chain at event based on event horizon
 * and log events dependent on it
 * and commit them to the log
 */
int LogResolveHorizon(GLOG *gl, EVENT *event)
{
	unsigned long host_id;
	unsigned long long seq_no;
	HOST *host;
	int err;
	EVENT *new_ev;
	EVENT *this_event;
	int done;


	/*
	 * reacquire the lock
	 */
	if(gl == NULL) {
		return(-1);
	}

	if(event == NULL) {
		return(-1);
	}

	host = HostListFind(gl->host_list,event->cause_host);
	if(host == NULL) {
		return(1);
	}

	this_event = PendingFindEvent(gl->pending,
			event->host,
			event->seq_no);

	/*
	 * if the event is missing, skip
	 */
	if(this_event == NULL){
		fprintf(stderr,"event %lu %llu not on pending list\n",
			event->host,
			event->seq_no);
		fflush(stderr);
		return(-1);
	}

	/*
	 * if this event is on the pending list and it is earlier than the
	 * event horizon, make the event horizon the cause_seq_no and log
	 * it
	 */
	if(this_event->cause_seq_no < host->event_horizon) {
		/*
		 * save these off so we can remove it from pending
		 * list
		 */
		new_ev = EventCreate(this_event->type,
				     this_event->host);
		if(new_ev == NULL) {
			fprintf(stderr,"no spave for new ev\n");
			fflush(stderr);
			return(-1);
		}
#ifdef DEBUG
printf("logresolve %lu: terminating ev %lu %llu cause %lu %llu max: %llu eh: %llu\n",
		gl->host_id,
		this_event->host,
		this_event->seq_no,
		this_event->cause_host,
		this_event->cause_seq_no,
		host->max_seen,
		host->event_horizon);
		fflush(stdout);
#endif
		/*
		 * make the event self enabling
		 */
		new_ev->seq_no = this_event->seq_no;
		new_ev->cause_host = host->host_id;
		new_ev->cause_seq_no = host->event_horizon;
		/*
		 * pull it form the pending list (which is destructive)
		 */
		err = PendingRemoveEvent(gl->pending,this_event);
		if(err < 0) {
			fprintf(stderr,
"couldn't remove pending event %lu %llu with max %llu and horizon %llu\n",
				new_ev->host,
				new_ev->seq_no,
				host->max_seen,
				host->event_horizon);
			fflush(stderr);
			EventFree(new_ev);
			return(-1);
		} else { /* log it */
			/*
			 * drop the lock here since we have
			 * it off the list
			 */
			err = GLogEvent(gl,new_ev);
			if(err < 0) {
				fprintf(stderr,
"couldn't log ff event %lu %llu with max %llu and horizon %llu\n",
				new_ev->host,
				new_ev->seq_no,
				host->max_seen,
				host->event_horizon);
				fflush(stderr);
				EventFree(new_ev);
				return(-1);
			}
			return(1);
		}

	}
	return(1);
}

void GLogPrintPending(GLOG *gl)
{
	RB *rb;
	EVENT *ev;
	HOST *cause_host;
	HOST *host;


	/*
	 * otherwise check current events
	 */
	RB_FORWARD(gl->pending->alive,rb) {
		host = HostListFind(gl->host_list,ev->host);
		cause_host = HostListFind(gl->host_list,ev->cause_host);
		ev = (EVENT *)rb->value.v;
		printf("glog %lu: psat %lu %llu (max: %llu eh: %llu) %lu %llu (max: %llu eh: %llu)\n",
			gl->host_id,
			ev->host,
			ev->seq_no,
			host->max_seen,
			host->event_horizon,
			ev->cause_host,
			ev->cause_seq_no,
			cause_host->max_seen,
			cause_host->event_horizon);
		fflush(stdout);
	}

	return;
}
/*
 * multiple events with the same cause leave events with cause_seq_no
 * <= max_seen
 */
EVENT *GLogPendingSatisfied(GLOG *gl)
{
	RB *rb;
	EVENT *ev;
	HOST *cause_host;
	HOST *host;


	/*
	 * otherwise check current events
	 */
	RB_FORWARD(gl->pending->alive,rb) {
		ev = (EVENT *)rb->value.v;
		cause_host = HostListFind(gl->host_list,ev->cause_host);
		if(cause_host == NULL) {
			continue;
		}
		if(cause_host->max_seen >= ev->cause_seq_no) {
#ifdef DEBUG
printf("pendsatisfied %lu: ev with cause %lu %llu cause %lu %llu cause max: %llu\n",
		gl->host_id,
		ev->host,
		ev->seq_no,
		ev->cause_host,
		ev->cause_seq_no,
		cause_host->max_seen);
		fflush(stdout);
#endif
			return(ev);
		}

	}

	return(NULL);
}

int IsAnchor(EVENT *ev) {
	if((ev->host == ev->cause_host) && 
	   (ev->seq_no == ev->cause_seq_no)) {
		return(1);
	} else {
		return(0);
	}
}

int GLogEvent(GLOG *gl, EVENT *event)
{
	double ndx;
	unsigned long host_id;
	unsigned long long seq_no;
	unsigned long long cause_seq_no;
	RB *rb;
	HOST *cause_host;
	HOST *host;
	int err;
	EVENT *cause_event;
	EVENT *first_ev;
	EVENT *local_event;
	EVENT *this_event;
	int done;
	EVENT *ev;
	int added;
	int event_logged = 0;

	if(gl == NULL) {
		return(-1);
	}

	if(event == NULL) {
		return(-1);
	}


	/*
	 * see if the cause has already been logged
	 */
	cause_host = HostListFind(gl->host_list,event->cause_host);

	/*
	 * if this is our first message from this host, add the host and
	 * record seq no
	 */
	if(cause_host == NULL) {
		err = HostListAdd(gl->host_list,event->cause_host);
		if(err < 0) {
			fprintf(stderr,"couldn't add new cause host %lu\n",
				event->cause_host);
			fflush(stderr);
			return(-1);
		}
		cause_host = HostListFind(gl->host_list,event->cause_host);
		if(cause_host == NULL) {
			fprintf(stderr,"couldn't find after add of new cause host %lu\n",
				event->cause_host);
			fflush(stderr);
			return(-1);
		}
		/*
		 * make a first event
		 */
		first_ev = EventCreate(UNKNOWN,event->cause_host);
		if(first_ev == NULL) {
			fprintf(stderr,"couldn't make first event\n");
			return(-1);
		}
		first_ev->seq_no = event->cause_seq_no;
		err = LogAdd(gl->log,first_ev);
		EventFree(first_ev);
		if(err < 0) {
			fprintf(stderr,"couldn't add first event\n");
			return(-1);
		}
		cause_host->max_seen = event->cause_seq_no;
	} 

	/*
	 * now check to see if the event host is present
	 */
	host = HostListFind(gl->host_list,event->host);

	/*
	 * if this is the first event, log it
	 */
	if(host == NULL) {
		err = HostListAdd(gl->host_list,event->host);
		if(err < 0) {
			fprintf(stderr,"couldn't add new cause host %lu\n",
				event->host);
			fflush(stderr);
			return(-1);
		}
		host = HostListFind(gl->host_list,event->host);
		if(host == NULL) {
			fprintf(stderr,"couldn't find after add of new cause host %lu\n",
				event->host);
			fflush(stderr);
			return(-1);
		}
		host->max_seen = event->seq_no;

		err = LogAdd(gl->log,event);
		if(err < 0) {
			fprintf(stderr,"couldn't log after add of new host %lu\n",
				event->host);
			fflush(stderr);
			return(-1);
		}
		return(err);
	} else { /* we have seen the host, have we seen enough events? */
#ifdef DEBUG
printf("glog %lu: ev: %lu %llu, max: %llu eh: %llu check up to date\n",
		gl->host_id, 
		event->host, 
		event->seq_no, 
		host->max_seen, 
		host->event_horizon);
		fflush(stdout);
#endif
		if(host->max_seen >= event->seq_no) {
#ifdef DEBUG
printf("glog %lu: ev: %lu %llu, max: %llu eh: %llu is up to date\n",
		gl->host_id, 
		event->host, 
		event->seq_no, 
		host->max_seen, 
		host->event_horizon);
		fflush(stdout);
#endif
			return(1);
		}
	}


	/*
	 * there a pending cause?
	 */
	cause_event = PendingFindEvent(gl->pending,event->cause_host,
			event->cause_seq_no);

	if((cause_event != NULL) && IsAnchor(event)) {
		err = PendingRemoveEvent(gl->pending,cause_event);
		if(err < 0) {
			fprintf(stderr,"couldn't remove anchor cause\n");
			fflush(stderr);
			return(-1);
		}
		cause_event = NULL;
	}
	if(cause_event != NULL) {
#ifdef DEBUG
printf("glog %lu: pending add %lu %llu because of pending cause %lu %llu\n",
		gl->host_id,
		event->host,
		event->seq_no,
		event->cause_host,
		event->cause_seq_no);

		fflush(stdout);
#endif
		err = PendingAddEvent(gl->pending,event);
		return(err);
	}

	cause_host = HostListFind(gl->host_list,event->cause_host);

	/*
	 * if cause host is not up to date, put this event on the pending
	 * list
	 */
	if((cause_host->max_seen < event->cause_seq_no) && 
	   (cause_host->event_horizon < event->cause_seq_no) &&
			!IsAnchor(event)) {
#ifdef DEBUG
printf("glog %lu: pending %lu %llu because of waiting cause %lu %llu ",
		gl->host_id,
		event->host,
		event->seq_no,
		event->cause_host,
		event->cause_seq_no);
printf("max: %llu eh: %llu\n",
		cause_host->max_seen,
		cause_host->event_horizon);
		fflush(stdout);
#endif
		err = PendingAddEvent(gl->pending,event);
		return(err);
	}

	/*
	 * otherwise, log this event
	 */
#ifdef DEBUG
printf("glog %lu: adding %lu %llu with cause %lu %llu\n",
		gl->host_id,
		event->host,
		event->seq_no,
		event->cause_host,
		event->cause_seq_no);
		fflush(stdout);
#endif
	err = LogAdd(gl->log,event);
	if(err < 0) {
		fprintf(stderr,"glog %lu couldn't add event %lu %llu\n",
				gl->host_id,
				event->host,
				event->seq_no);
		fflush(stderr);
		return(-1);
	} 
	event_logged = 1;

	if((host->max_seen < event->seq_no) || IsAnchor(event)) {
		host->max_seen = event->seq_no;
	}

	/*
	 * walk forward logging events that have been enabled
	 */
	this_event = PendingFindCause(gl->pending,event->host,event->seq_no);
	while(this_event != NULL) {
#ifdef DEBUG
printf("glog %lu: next event considered: %lu %llu\n",
		gl->host_id,
		this_event->host,
		this_event->seq_no);
		fflush(stdout);
#endif
		cause_host = HostListFind(gl->host_list,
				this_event->cause_host);
		if(cause_host == NULL) {
			err = HostListAdd(gl->host_list,event->cause_host);
			if(err < 0) {
				fprintf(stderr,
					"couldn't add cause host %lu\n",
					this_event->cause_host);
				fflush(stderr);
				return(-1);
			}
			cause_host = HostListFind(gl->host_list,
						this_event->cause_host);
#ifdef DEBUG
printf("glog %lu: adding cause_host ev %lu %llu with cause %lu %llu\n",
		gl->host_id,
		this_event->host,
		this_event->seq_no,
		this_event->cause_host,
		this_event->cause_seq_no);
		fflush(stdout);
#endif
			err = LogAdd(gl->log,this_event);
			if(err < 0) {
				fprintf(stderr,
		"glog %lu couldn't add new pending cause event %lu %llu\n",
						gl->host_id,
						this_event->host,
						this_event->seq_no);
				fflush(stderr);
				return(-1);
			} 
			host_id = this_event->host;
			seq_no = this_event->seq_no;
			cause_seq_no = this_event->cause_seq_no;
			err = PendingRemoveEvent(gl->pending,this_event);
			if(err < 0) {
				fprintf(stderr,
		"glog %lu couldn't remove pending cause event %lu %llu\n",
						gl->host_id,
						this_event->host,
						this_event->seq_no);
				fflush(stderr);
				return(-1);
			}
			cause_host->max_seen = cause_seq_no;
			this_event = PendingFindCause(gl->pending,
					host_id,seq_no);
			continue;
		}
		/*
		 * otherwise, there is a cause host
		 *
		 * check to see if this event is now enabled and add it
		 * if it is
		 */
		if(cause_host->max_seen >= this_event->cause_seq_no) {
#ifdef DEBUG
printf("glog %lu: adding now enabled pending %lu %llu with cause %lu %llu\n",
		gl->host_id,
		this_event->host,
		this_event->seq_no,
		this_event->cause_host,
		this_event->cause_seq_no);
		fflush(stdout);
#endif
			err = LogAdd(gl->log,this_event);
			if(err < 0) {
				fprintf(stderr,
		"glog %lu couldn't add new enabled cause event %lu %llu\n",
						gl->host_id,
						this_event->host,
						this_event->seq_no);
				fflush(stderr);
				return(-1);
			}
			host_id = this_event->host;
			seq_no = this_event->seq_no;
			err = PendingRemoveEvent(gl->pending,this_event);
			if(err < 0) {
				fprintf(stderr,
		"glog %lu couldn't remove new pending cause event %lu %llu\n",
						gl->host_id,
						this_event->host,
						this_event->seq_no);
				fflush(stderr);
				return(-1);
			}
			host = HostListFind(gl->host_list, host_id);
			if(host->max_seen < seq_no) {
				host->max_seen = seq_no;
			}
			this_event = PendingFindCause(gl->pending,
					host_id,seq_no);

				/*
		 		 * check to see if there are satisfied forks
		 		 */
			if(this_event == NULL) {
				this_event = GLogPendingSatisfied(gl);
			}
			continue;
		}

		/*
		 * otherwise we have reached the end of the enabled
		 * chain
		 */
		break;
	}

	return(1);
}

	
void GLogPrint(FILE *fd, GLOG *gl)
{
	LogPrint(fd,gl->log);
	PendingPrint(fd,gl->pending);
	return;
}


/*
 * bring clear pending list of events earlier than event horizon
 */
int GLogFastForward(GLOG *gl, unsigned long remote_host_id)
{
	RB *rb;
	EVENT *event;
	int err;
	HOST *remote_host;

	remote_host = HostListFind(gl->host_list,remote_host_id);
	if(remote_host == NULL) {
		/*
		 * no remote host yet means no event horizon
		 */
		return(1);
	}

	if(remote_host->max_seen >= remote_host->event_horizon) { 
		/* we are good */
#ifdef DEBUG
printf("glogfastforward %lu: host %lu max: %llu eh: %llu, good\n",
		gl->host_id,
		remote_host->host_id,
		remote_host->max_seen,
		remote_host->event_horizon);
		fflush(stdout);
#endif
		return(1);
	}

	/*
	 * clean out the list.  Find events that depend on events
	 * from this host.  Note that RB code is destructive on delete
	 * so must scan until there are no more to do
	 */
	P(&gl->mutex);
	rb = RB_FIRST(gl->pending->alive);
	
	if(rb == NULL) {
		/*
		 * nothing on the pending list
		 */
#ifdef DEBUG
printf("glogfastforward %lu: host %lu max: %llu eh: %llu, nothing pending\n",
		gl->host_id,
		remote_host->host_id,
		remote_host->max_seen,
		remote_host->event_horizon);
		fflush(stdout);
#endif
		V(&gl->mutex);
		return(1);
	} 
	while(rb != NULL) {
		event = (EVENT *)rb->value.v;
#ifdef DEBUG
printf("glogfastforward %lu: pending %lu %llu cause %lu %llu max: %llu eh: %llu\n",
		gl->host_id,
		event->host,
		event->seq_no,
		event->cause_host,
		event->cause_seq_no,
		remote_host->max_seen,
		remote_host->event_horizon);
		fflush(stdout);
#endif
		if(event->cause_host != remote_host->host_id) {
			rb = rb->next;
			continue;
		}

		if(event->cause_seq_no >= remote_host->event_horizon) {
			rb = rb->next;
			continue;
		}
		/*
		 * if we are past the event horzon, then
		 * we are done fast forwarding
		 */
#ifdef DEBUG
printf("glogfastforward %lu: event %lu %llu rhost: %lu eh: %llu\n",
		gl->host_id, 
		event->host, 
		event->seq_no,
		remote_host->host_id, 
		remote_host->event_horizon);
		fflush(stdout);
#endif
		err = LogResolveHorizon(gl,event);
		if(err < 0) {
			fprintf(stderr,"fast forward of %lu %llu failed\n",
				event->host,
				event->seq_no);
			V(&gl->mutex);
			return(-1);
		}
		/*
		 * FIX ME
		 * could be faster if starting from first element
		 * in pending belonging to remote host
		 */
		rb = RB_FIRST(gl->pending->alive);
	}

	V(&gl->mutex);
	return(1);

}
		
		


	

	


/*
 * add events to GLOG from log tail extracted from some local log
 */
int ImportLogTail(GLOG *gl, LOG *ll) 
{
	LOG *lt;
	unsigned long long earliest;
	unsigned long long event_horizon;
	HOST *host;
	unsigned long curr;
	unsigned long last;
	unsigned long first;
	EVENT *ev;
	int err;
	unsigned long remote_host;

	if(ll == NULL) {
		return(-1);
	}

	if(gl == NULL) {
		return(-1);
	}

	remote_host = ll->host_id;

	/*
	 * find the max seq_no from the remote gost that the GLOG has seen
	 */
	host = HostListFind(gl->host_list,remote_host);
	if(host == NULL) {
		earliest = 0;
	} else {
		earliest = host->max_seen;
	}



	/*
	 * lock the local log while we extract the tail
	 */
	P(&ll->mutex);
#ifdef DEBUG
printf("import %lu: from %lu, earliest %llu\n",
		gl->host_id,
		ll->host_id,
		earliest);
		fflush(stdout);
#endif
	lt = LogTail(ll,earliest,ll->size);
	if(lt == NULL) {
		/*
		 * there is no local log tail we haven't seen
		 */
#ifdef DEBUG
printf("import %lu: from %lu, earliest %llu no tail\n",
		gl->host_id,
		ll->host_id,
		earliest);
		fflush(stdout);
#endif
		V(&ll->mutex);
		return(0);
	}
	V(&ll->mutex);

	if(lt->head == lt->tail) {
		return(0);
	}

	ev = (EVENT *)(MIOAddr(lt->m_buf) + sizeof(LOG));

	/*
	 * set the event horizon
	 */
	first = lt->head;
	last = lt->tail + 1;
	if(last >= lt->size) {
		last = 0;
	}
	if(host != NULL) {
#ifdef DEBUG
printf("import %lu: from %lu, earliest %llu setting eh to %llu\n",
		gl->host_id,
		ll->host_id,
		earliest,
		ev[first].seq_no);
printf("import %lu: from %lu, first %llu last %llu\n",
		gl->host_id,
		ll->host_id,
		ev[first].seq_no,
		ev[last].seq_no);
		fflush(stdout);
#endif
		if(ev[first].seq_no > host->event_horizon) {
			host->event_horizon = ev[first].seq_no;
		}
		err = GLogFastForward(gl,remote_host);
		if(err < 0) {
			fprintf(stderr,
			"couldn't fast forward from host %lu\n",
				remote_host);
			fflush(stderr);
			LogFree(lt);
			return(-1);
		}
	}

	/*
	 * now do the import if there is something to import
	 */
	last = curr = lt->head;
	P(&gl->mutex);
	while(ev[curr].seq_no >= earliest) {
#ifdef DEBUG
printf("import %lu: logging from %lu: host: %lu seq_no %llu\n",
		gl->host_id, 
		ll->host_id, 
		ev[curr].host,
		ev[curr].seq_no);
		fflush(stdout);
#endif
		err = GLogEvent(gl,&ev[curr]);
		if(err < 0) {
			fprintf(stderr,"couldn't add seq_no %llu\n",
				ev[curr].seq_no);
			fflush(stderr);
			LogFree(lt);
			V(&gl->mutex);
			return(0);
		}
		last = curr;
		curr = curr - 1;
		if(curr >= lt->size) {
			curr = (lt->size-1);
		}
		if(curr == lt->tail) {
			break;
		}
	}

	LogFree(lt);
	V(&gl->mutex);

	return(1);
}
	
	
/* 
FireEvent logs event to local log and to global log
ProcessEventRecord puts record in global log

fetch deps function: max seen, max pending
*/
