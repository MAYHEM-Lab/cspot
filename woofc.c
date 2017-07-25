#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "log.h"
#include "woofc.h"

extern char WooF_dir[2048];
extern char Host_log_name[2048];
extern unsigned long Host_id;
extern LOG *Host_log;

#define DEBUG


int WooFCreate(char *name,
	       unsigned long element_size,
	       unsigned long history_size)
{
	WOOF *wf;
	MIO *mio;
	unsigned long space;
	char local_name[4096];
	char fname[20];

	/*
	 * each element gets a seq_no and log index so we can handle
	 * function cancel if we wrap
	 */
	space = ((history_size+1) * (element_size +sizeof(ELID))) + 
			sizeof(WOOF);

	if(WooF_dir == NULL) {
		fprintf(stderr,"WooFCreate: must init system\n");
		fflush(stderr);
		exit(1);
	}

	memset(local_name,0,sizeof(local_name));
	strncpy(local_name,WooF_dir,sizeof(local_name));
	if(local_name[strlen(local_name)-1] != '/') {
		strncat(local_name,"/",1);
	}


	if(name != NULL) {
		strncat(local_name,name,sizeof(local_name));
	} else {
		sprintf(fname,"woof-%10.0",drand48()*399999999);
		strncat(local_name,fname,sizeof(fname));
	}
	mio = MIOOpen(local_name,"w+",space);
	if(mio == NULL) {
		return(-1);
	}
#ifdef DEBUG
	printf("WooFCreate: opened %s\n",local_name);
	fflush(stdout);
#endif

	wf = (WOOF *)MIOAddr(mio);
	memset(wf,0,sizeof(WOOF));
	wf->mio = mio;

	if(name != NULL) {
		strncpy(wf->filename,name,sizeof(wf->filename));
	} else {
		strncpy(wf->filename,fname,sizeof(wf->filename));
	}

	memset(wf->handler_name,0,sizeof(wf->handler_name));
	strncpy(wf->handler_name,name,sizeof(wf->handler_name));
	strncat(wf->handler_name,".handler",sizeof(wf->handler_name)-strlen(wf->handler_name));
	wf->history_size = history_size;
	wf->element_size = element_size;
	wf->seq_no = 1;

	InitSem(&wf->mutex,1);
	InitSem(&wf->tail_wait,0);

	MIOClose(wf->mio);

	return(1);
}

WOOF *WooFOpen(char *name)
{
	WOOF *wf;
	MIO *mio;
	char local_name[4096];

	if(name == NULL) {
		return(NULL);
	}

	if(WooF_dir == NULL) {
		fprintf(stderr,"WooFOpen: must init system\n");
		fflush(stderr);
		exit(1);
	}

	memset(local_name,0,sizeof(local_name));
	strncpy(local_name,WooF_dir,sizeof(local_name));
	if(local_name[strlen(local_name)-1] != '/') {
		strncat(local_name,"/",1);
	}


	strncat(local_name,name,sizeof(local_name));
#ifdef DEBUG
	printf("WooFOpen: trying to open %s\n",local_name);
	fflush(stdout);
#endif
	mio = MIOReOpen(local_name);
	if(mio == NULL) {
		return(NULL);
	}
#ifdef DEBUG
	printf("WooFOpen: opened %s\n",local_name);
	fflush(stdout);
#endif

	wf = (WOOF *)MIOAddr(mio);

	return(wf);
}

void WooFFree(WOOF *wf)
{
	MIOClose(wf->mio);

	return;
}

		
int WooFPut(char *wf_name, char *hand_name, void *element)
{
	MIO *mio;
	MIO *lmio;
	LOG *host_log;
	WOOF *wf;
	char woof_name[2048];
	char log_name[4096];
	pthread_t tid;
	unsigned long next;
	unsigned char *buf;
	unsigned char *ptr;
	ELID *el_id;
	unsigned long seq_no;
	unsigned long ndx;
	int err;
	char launch_string[4096];
	char *host_log_seq_no;
	unsigned long my_log_seq_no;
	EVENT *ev;
	unsigned long ls;
#ifdef DEBUG
	printf("WooFPut: called\n");
	fflush(stdout);
#endif

	if(WooF_dir[0] == 0) {
		fprintf(stderr,"WooFPut: must init system\n");
		fflush(stderr);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFPut: WooF_dir: %s\n",WooF_dir);
	fflush(stdout);
#endif

	memset(woof_name,0,sizeof(woof_name));
	strncpy(woof_name,WooF_dir,sizeof(woof_name));
	strncat(woof_name,"/",sizeof(woof_name)-strlen("/"));
	strncat(woof_name,wf_name,sizeof(woof_name)-strlen(woof_name));

	wf = WooFOpen(wf_name);

#ifdef DEBUG
	printf("WooFPut: WooF %s open\n",wf_name);
	fflush(stdout);
#endif

	/*
	 * if called from within a handler, env variable carries cause seq_no
	 * for logging
	 *
	 * when called for first time on host to start, should be NULL
	 */
	host_log_seq_no = getenv("WOOF_HOST_LOG_SEQNO");
	if(host_log_seq_no != NULL) {
		my_log_seq_no = atol(host_log_seq_no);
	} else {
		my_log_seq_no = 1;
	}

	if(hand_name != NULL) {
		ev = EventCreate(TRIGGER,Host_id);
		if(ev == NULL) {
			fprintf(stderr,"WooFPut: couldn't create log event\n");
			exit(1);
		}
		ev->cause_host = Host_id;
		ev->cause_seq_no = my_log_seq_no;
	}
	

#ifdef DEBUG
	printf("WooFPut: adding element\n");
	fflush(stdout);
#endif
	/*
	 * now drop the element in the object
	 */
	P(&wf->mutex);
#ifdef DEBUG
	printf("WooFPut: in mutex\n");
	fflush(stdout);
#endif
	/*
	 * find the next spot
	 */
	next = (wf->head + 1) % wf->history_size;
	ndx = next;

	/*
	 * write the data and record the indices
	 */
	buf = (unsigned char *)(((void *)wf) + sizeof(WOOF));
	ptr = buf + (next * (wf->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wf->element_size);

	/*
	 * tail is the last valid place where data could go
	 * check to see if it is allocated to a function that
	 * has yet to complete
	 */
#ifdef DEBUG
	printf("WooFPut: element in\n");
	fflush(stdout);
#endif
	while((el_id->busy == 1) && (next == wf->tail)) {
#ifdef DEBUG
	printf("WooFPut: busy at %lu\n",next);
	fflush(stdout);
#endif
		V(&wf->mutex);
		P(&wf->tail_wait);
		P(&wf->mutex);
		next = (wf->head + 1) % wf->history_size;
		ptr = buf + (next * (wf->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr+wf->element_size);
	}

	/*
	 * write the element
	 */
#ifdef DEBUG
	printf("WooFPut: writing element 0x%x\n",el_id);
	fflush(stdout);
#endif
	memcpy(ptr,element,wf->element_size);
	/*
	 * and elemant meta data after it
	 */
	el_id->seq_no = wf->seq_no;
	el_id->busy = 1;

	/*
	 * update circular buffer
	 */
	ndx = wf->head = next;
	if(next == wf->tail) {
		wf->tail = (wf->tail + 1) % wf->history_size;
	}
	seq_no = wf->seq_no;
	wf->seq_no++;
	V(&wf->mutex);
#ifdef DEBUG
	printf("WooFPut: out of element mutex\n");
	fflush(stdout);
#endif

	/*
	 * if there is no handler, we are done (no need to generate log record)
	 */
	if(hand_name == NULL) {
		return(1);
	}

	ev->woofc_ndx = ndx;
#ifdef DEBUG
	printf("WooFPut: ndx: %lu\n",ev->woofc_ndx);
	fflush(stdout);
#endif
	ev->woofc_seq_no = seq_no;
#ifdef DEBUG
	printf("WooFPut: seq_no: %lu\n",ev->woofc_seq_no);
	fflush(stdout);
#endif
	ev->woofc_element_size = wf->element_size;
#ifdef DEBUG
	printf("WooFPut: element_size %lu\n",ev->woofc_element_size);
	fflush(stdout);
#endif
	ev->woofc_history_size = wf->history_size;
#ifdef DEBUG
	printf("WooFPut: history_size %lu\n",ev->woofc_history_size);
	fflush(stdout);
#endif
	memset(ev->woofc_name,0,sizeof(ev->woofc_name));
	strncpy(ev->woofc_name,wf->filename,sizeof(ev->woofc_name));
#ifdef DEBUG
	printf("WooFPut: name %s\n",ev->woofc_name);
	fflush(stdout);
#endif
	memset(ev->woofc_handler,0,sizeof(ev->woofc_handler));
	strncpy(ev->woofc_handler,hand_name,sizeof(ev->woofc_handler));
#ifdef DEBUG
	printf("WooFPut: handler %s\n",ev->woofc_handler);
	fflush(stdout);
#endif

	/*
	 * log the event so that it can be triggered
	 */
	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s/%s",WooF_dir,Host_log_name);
#ifdef DEBUG
	printf("WooFPut: logging event to %s\n",log_name);
	fflush(stdout);
#endif
	ls = LogEvent(Host_log,ev);
	if(ls == 0) {
		fprintf(stderr,"WooFPut: couldn't log event to log %s\n",
			log_name);
		fflush(stderr);
		return(-1);
	}

#ifdef DEBUG
	printf("WooFPut: logged %lu for woof %s %s\n",
		ls,
		ev->woofc_name,
		ev->woofc_handler);
	fflush(stdout);
#endif

	EventFree(ev);
//	lmio = MIOReOpen(log_name);
//	host_log = (LOG *)MIOAddr(lmio);
	V(&Host_log->tail_wait);
//	MIOClose(mio);
//	MIOClose(lmio);
	return(1);
}

int WooFGetTail(WOOF *wf, void *elements, int element_count)
{
	int i;
	unsigned long ndx;
	unsigned char *buf;
	unsigned char *ptr;
	unsigned char *lp;

	buf = (unsigned char *)(((void *)wf) + sizeof(WOOF));
	
	i = 0;
	lp = (unsigned char *)elements;
	P(&wf->mutex);
		ndx = wf->head;
		while(i < element_count) {
			ptr = buf + (ndx * (wf->element_size + sizeof(ELID)));
			memcpy(lp,ptr,wf->element_size);
			lp += wf->element_size;
			i++;
			ndx = ndx - 1;
			if(ndx >= wf->history_size) {
				ndx = 0;
			}
			if(ndx == wf->tail) {
				break;
			}
		}
	V(&wf->mutex);

	return(i);

}

int WooFGet(WOOF *wf, void *element, unsigned long ndx)
{
	unsigned char *buf;
	unsigned char *ptr;

	/*
	 * must be a valid index
	 */
	if(ndx >= wf->history_size) {
		return(-1);
	}
	buf = (unsigned char *)(((void *)wf) + sizeof(WOOF));
	ptr = buf + (ndx * (wf->element_size + sizeof(ELID)));
	memcpy(element,ptr,sizeof(wf->element_size));
	return(1);
}

unsigned long WooFEarliest(WOOF *wf)
{
	unsigned long earliest;

	earliest = (wf->tail + 1) % wf->history_size;
	return(earliest);
}

unsigned long WooFLatest(WOOF *wf)
{
	return(wf->head);
}

unsigned long WooFNext(WOOF *wf, unsigned long ndx)
{
	unsigned long next;

	next = (ndx + 1) % wf->history_size;

	return(next);
}

unsigned long WooFBack(WOOF *wf, unsigned long elements)
{
	unsigned long remainder = elements % wf->history_size;
	unsigned long new;
	unsigned long wrap;

	if(elements == 0) {
		return(wf->head);
	}

	new = wf->head - remainder;

	/*
	 * if we need to wrap around
	 */
	if(new >= wf->history_size) {
		wrap = remainder - wf->head;
		new = wf->history_size - wrap;
	}

	return(new);
}
		

	


