#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
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

int LogAdd(LOG *log, EVENT *event)
{
	EVENT *ev_array;
	unsigned long int next;
	unsigned long long seq_no;

	if(log == NULL) {
		return(1);
	}

	ev_array = (EVENT *)(MIOAddr(log->m_buf) + sizeof(LOG));

//	pthread_mutex_lock(&log->lock);
	/*
	 * FIX ME: need to implement a cleaner that moves the tail
	 */
	if(LogFull(log)) {
		log->tail = (log->tail + 1) % log->size;
	}

	next = (log->head + 1) % log->size;
	
	memcpy(&ev_array[next],event,sizeof(EVENT));
	log->head = next;
//	pthread_mutex_unlock(&log->lock);

	return(1);
}

unsigned long long LogEvent(LOG *log, EVENT *event)
{
	int err;
	unsigned long long seq_no;

	if(log == NULL) {
		return(0);
	}

	seq_no = log->seq_no;
	event->seq_no = seq_no;
	log->seq_no++;
	err = LogAdd(log,event);
	
	return(seq_no);
}

LOG *LogTail(LOG *log, unsigned long long earliest, unsigned long max_size)
{
	LOG *log_tail;
	unsigned long curr;
	EVENT *ev_array;
	unsigned long count;

	if(log->head == log->tail) { /* log is empty */
		return(NULL);
	}

	log_tail = LogCreate(NULL,max_size);
	if(log_tail == NULL) {
		return(NULL);
	}

	ev_array = (EVENT *)(MIOAddr(log->m_buf) + sizeof(LOG));

	/*
	 * put events in the tail log in reverse order to save a scan
	 */
	count = 0;
	curr = log->head;
	while(ev_array[curr].seq_no >= earliest) {
		/*
		 * allow for ealiest to overlap
		 */
		if(ev_array[curr].seq_no > earliest) {
			LogAdd(log_tail,&ev_array[curr]);
			count++;
			if(count >= max_size) {
				break;
			}
		}
		if(curr == log->tail) { /* this was the last valid record */
			break;
		}
		curr = curr - 1;
		if(curr >= log->size) {
			curr = (log->size-1);
		}
	}

	return(log_tail);
}
		
	
void LogPrint(FILE *fd, LOG *log)
{
	unsigned long curr;
	EVENT *ev_array;

	ev_array = (EVENT *)(MIOAddr(log->m_buf) + sizeof(LOG));

	fprintf(fd,"log filename: %s\n",log->filename);
	fprintf(fd,"log size: %lu\n",log->size);
	fprintf(fd,"log head: %lu\n",log->head);
	fprintf(fd,"log tail: %lu\n",log->tail);

	curr = log->head;
	while(curr != log->tail) {
		fprintf(fd,
		"\thost: %lu seq_no: %llu r_host: %lu r_seq_no: %llu\n",
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
	EVENT *local_event;
	double ndx;

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
	local_event->seq_no = 0; /* marks MIO space as free */
	RBDeleteD(pending->alive,rb);

	ndx = EventIndex(event->cause_host,event->cause_seq_no);
	rb = RBFindD(pending->causes,ndx);
	if(rb == NULL) {
		fprintf(stderr,
		"couldn't find %lu %llu pending cause\n",
			event->host,
			event->seq_no);
		fflush(stderr);
		return(1);
	}

	RBDeleteD(pending->causes,rb);

	return(1);
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


GLOG *GLogCreate(char *filename, unsigned long size)
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
	gl->log = LogCreate(subfilename,size);
	if(gl->log == NULL) {
		GLogFree(gl);
		return(NULL);
	}

	memset(subfilename,0,sizeof(subfilename));
	strcpy(subfilename,filename);
	strcat(subfilename,".pending");
	gl->pending = PendingCreate(subfilename,size);
	if(gl->pending == NULL) {
		GLogFree(gl);
		return(NULL);
	}

	pthread_mutex_init(&gl->lock,NULL);

	return(gl);
}

void GLogFree(GLOG *gl)
{
	if(gl->host_list != NULL) {
		HostListFree(gl->host_list);
	}

	if(gl->log != NULL) {
		LogFree(gl->log);
	}

	if(gl->pending != NULL) {
		PendingFree(gl->pending);
	}

	return;
}

int GLogEvent(GLOG *gl, EVENT *event)
{
	double ndx;
	unsigned long host_id;
	unsigned long long seq_no;
	RB *rb;
	HOST *cause_host;
	HOST *host;
	int err;
	EVENT *cause_event;
	EVENT *first_ev;
	EVENT *local_event;
	EVENT *this_event;
	int done;

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
		if(host->max_seen >= event->seq_no) {
			return(1);
		}
	}

	/*
	 * here we have an event that is not the first event we've seen from
	 * this host
	 *
	 * walk the dependency list
	 */
	done = 0;
	this_event = event;
	while(done == 0) {
		cause_event =
			PendingFindEvent(gl->pending,this_event->cause_host,this_event->cause_seq_no);

		/*
		 * if we don't find the cause on the pending list, check to
		 * see if it is already logged
		 */
		if(cause_event == NULL) {
			cause_host = HostListFind(gl->host_list,this_event->cause_host);
			if(cause_host == NULL) {
				fprintf(stderr,"no pending cause but no host %lu\n",
						this_event->cause_host);
				fflush(stderr);
				return(-1);
			}
			/*
			 * if we have logged the cause already, but we
			 * haven't logged this event, log it
			 */
			if(cause_host->max_seen >= this_event->cause_seq_no) {
				err = LogAdd(gl->log,this_event);
				if(err < 0) {
					fprintf(stderr,
					"log with committed cause failed %lu, %llu\n",
						event->host,event->seq_no);
					fflush(stderr);
					return(-1);
				}


				host = HostListFind(gl->host_list,this_event->host);
				if(host == NULL) {
					fprintf(stderr,"cleared pending cause but no host %lu\n",
						this_event->host);
					fflush(stderr);
					return(-1);
				}
				/*
				 * record seq_no if it is valid
				 */
				if(host->max_seen < this_event->seq_no) {
					host->max_seen = this_event->seq_no;
				}
				/*
				 * now remove from pending list if it was
				 * there
				 *
				 * must save off seq_no and host num because
				 * remove resets host ID
			 	 */
				seq_no = this_event->seq_no;
				host_id = this_event->host;
				local_event = PendingFindEvent(gl->pending,
						this_event->host,
						this_event->seq_no);
				if(local_event != NULL) {
					err = PendingRemoveEvent(gl->pending,
						local_event);
					if(err < 0) {
						fprintf(stderr,
					"couldn't remove pending %lu %llu\n",
							local_event->host,
							local_event->seq_no);
						done = 1;
						continue;
					}
				}
				/*
			 	 * now find the next pending event
			 	 * that depends on this event
				 */
				this_event = PendingFindCause(gl->pending,
							host_id,
							seq_no);
				if(this_event == NULL) {
					done = 1;
				} 

#if 0
else {
					/*
					 * is this event already logged?
					 */
					host = HostListFind(gl->host_list,this_event->host);
					if((host != NULL) && 
						(host->max_seen >= this_event->seq_no)){ 
							PendingRemoveEvent(gl->pending,this_event);
						done = 1;
					}
				}
#endif
				continue;
			}

			/*
			 * are we current with the event's host?
			 *
			 * if so, bail out
			 */
			if(host->max_seen >= this_event->seq_no) {
				done = 1;
				continue;
			}

			/*
		 	 * otherwise, add the event to the pending list so we
		 	 * can wait for the cause to arrive
		 	 *
		 	 * if it is already on the pending list then it is
			 * okay since it may have come from a log import
		 	 * an error
		 	 */

			local_event = PendingFindEvent(gl->pending,event->host,event->seq_no);
			if(local_event == NULL) {
				err = PendingAddEvent(gl->pending,event);
			} else {
				err = 1;
			}
			done = 1;
			break;
		} else { /* cause is found on pending list */
			/*
			 * here the cause is on the pending list so this
			 * event needs to go on the list
			 *
			 * if it is there already then there is a problem
			 */
			local_event = PendingFindEvent(gl->pending,this_event->host,event->seq_no);
			if(local_event != NULL) {
				fprintf(stderr,
				"already pending cause for host %lu event %llu\n",
					this_event->host,this_event->seq_no);
				fflush(stderr);
				return(-1);
			}
			err = PendingAddEvent(gl->pending,this_event);
			done = 1;
			break;
		}
	} /* end of while loop */

	return(err);
}

void GLogPrint(FILE *fd, GLOG *gl)
{
	LogPrint(fd,gl->log);
	PendingPrint(fd,gl->pending);
	return;
}



	
/* 
FireEvent logs event to local log and to global log
ProcessEventRecord puts record in global log

fetch deps function: max seen, max pending
*/
