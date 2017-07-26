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



int WooFCreate(char *name,
	       unsigned long element_size,
	       unsigned long history_size)
{
	WOOF_SHARED *wf;
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

	wf = (WOOF_SHARED *)MIOAddr(mio);
	memset(wf,0,sizeof(WOOF_SHARED));

	if(name != NULL) {
		strncpy(wf->filename,name,sizeof(wf->filename));
	} else {
		strncpy(wf->filename,fname,sizeof(wf->filename));
	}

	wf->history_size = history_size;
	wf->element_size = element_size;
	wf->seq_no = 1;

	InitSem(&wf->mutex,1);
	InitSem(&wf->tail_wait,0);

	MIOClose(mio);

	return(1);
}

WOOF *WooFOpen(char *name)
{
	WOOF *wf;
	WOOF_SHARED *wfs;
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

	wf = (WOOF *)malloc(sizeof(WOOF));
	if(wf == NULL) {
		MIOClose(mio);
		return(NULL);
	}
	memset(wf,0,sizeof(WOOF));

	wf->shared = (WOOF_SHARED *)MIOAddr(mio);
	wf->mio = mio;

	return(wf);
}

void WooFFree(WOOF *wf)
{
	MIOClose(wf->mio);
	free(wf);

	return;
}

		
int WooFPut(char *wf_name, char *hand_name, void *element)
{
	MIO *mio;
	MIO *lmio;
	LOG *host_log;
	WOOF *wf;
	WOOF_SHARED *wfs;
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

	if(wf == NULL) {
		return(-1);
	}

#ifdef DEBUG
	printf("WooFPut: WooF %s open\n",wf_name);
	fflush(stdout);
#endif

	wfs = wf->shared;

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
	P(&wfs->mutex);
#ifdef DEBUG
	printf("WooFPut: in mutex\n");
	fflush(stdout);
#endif
	/*
	 * find the next spot
	 */
	next = (wfs->head + 1) % wfs->history_size;
	ndx = next;

	/*
	 * write the data and record the indices
	 */
	buf = (unsigned char *)(((void *)wfs) + sizeof(WOOF_SHARED));
	ptr = buf + (next * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);

	/*
	 * tail is the last valid place where data could go
	 * check to see if it is allocated to a function that
	 * has yet to complete
	 */
#ifdef DEBUG
	printf("WooFPut: element in\n");
	fflush(stdout);
#endif
	while((el_id->busy == 1) && (next == wfs->tail)) {
#ifdef DEBUG
	printf("WooFPut: busy at %lu\n",next);
	fflush(stdout);
#endif
		V(&wfs->mutex);
		P(&wfs->tail_wait);
		P(&wfs->mutex);
		next = (wfs->head + 1) % wfs->history_size;
		ptr = buf + (next * (wfs->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr+wfs->element_size);
	}

	/*
	 * write the element
	 */
#ifdef DEBUG
	printf("WooFPut: writing element 0x%x\n",el_id);
	fflush(stdout);
#endif
	memcpy(ptr,element,wfs->element_size);
	/*
	 * and elemant meta data after it
	 */
	el_id->seq_no = wfs->seq_no;
	el_id->busy = 1;

	/*
	 * update circular buffer
	 */
	ndx = wfs->head = next;
	if(next == wfs->tail) {
		wfs->tail = (wfs->tail + 1) % wfs->history_size;
	}
	seq_no = wfs->seq_no;
	wfs->seq_no++;
	V(&wfs->mutex);
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
	ev->woofc_element_size = wfs->element_size;
#ifdef DEBUG
	printf("WooFPut: element_size %lu\n",ev->woofc_element_size);
	fflush(stdout);
#endif
	ev->woofc_history_size = wfs->history_size;
#ifdef DEBUG
	printf("WooFPut: history_size %lu\n",ev->woofc_history_size);
	fflush(stdout);
#endif
	memset(ev->woofc_name,0,sizeof(ev->woofc_name));
	strncpy(ev->woofc_name,wfs->filename,sizeof(ev->woofc_name));
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
	WOOF_SHARED *wfs;

	wfs = wf->shared;

	buf = (unsigned char *)(((void *)wfs) + sizeof(WOOF_SHARED));
	
	i = 0;
	lp = (unsigned char *)elements;
	P(&wfs->mutex);
		ndx = wfs->head;
		while(i < element_count) {
			ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
			memcpy(lp,ptr,wfs->element_size);
			lp += wfs->element_size;
			i++;
			ndx = ndx - 1;
			if(ndx >= wfs->history_size) {
				ndx = 0;
			}
			if(ndx == wfs->tail) {
				break;
			}
		}
	V(&wfs->mutex);

	return(i);

}

int WooFGet(WOOF *wf, void *element, unsigned long ndx)
{
	unsigned char *buf;
	unsigned char *ptr;
	WOOF_SHARED *wfs;

	wfs = wf->shared;

	/*
	 * must be a valid index
	 */
	if(ndx >= wfs->history_size) {
		return(-1);
	}
	buf = (unsigned char *)(((void *)wfs) + sizeof(WOOF_SHARED));
	ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
	memcpy(element,ptr,sizeof(wfs->element_size));
	return(1);
}

unsigned long WooFEarliest(WOOF *wf)
{
	unsigned long earliest;
	WOOF_SHARED *wfs;

	wfs = wf->shared;

	earliest = (wfs->tail + 1) % wfs->history_size;
	return(earliest);
}

unsigned long WooFLatest(WOOF *wf)
{
	return(wf->shared->head);
}

unsigned long WooFNext(WOOF *wf, unsigned long ndx)
{
	unsigned long next;

	next = (ndx + 1) % wf->shared->history_size;

	return(next);
}

unsigned long WooFBack(WOOF *wf, unsigned long elements)
{
	WOOF_SHARED *wfs = wf->shared;
	unsigned long remainder = elements % wfs->history_size;
	unsigned long new;
	unsigned long wrap;

	if(elements == 0) {
		return(wfs->head);
	}

	new = wfs->head - remainder;

	/*
	 * if we need to wrap around
	 */
	if(new >= wfs->history_size) {
		wrap = remainder - wfs->head;
		new = wfs->history_size - wrap;
	}

	return(new);
}
		

