#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "log.h"
#include "woofc.h"

char WooF_dir[2048];
char Host_log_name[2048];
unsigned long Host_id;
LOG *Host_log;


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
	mio = MIOReOpen(local_name);
	if(mio == NULL) {
		return(NULL);
	}

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
	WOOF *wf;
	char woof_name[2048];
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

	if(WooF_dir == NULL) {
		fprintf(stderr,"WooFPut: must init system\n");
		fflush(stderr);
		return(-1);
	}

	memset(woof_name,0,sizeof(woof_name));
	strncpy(woof_name,WooF_dir,sizeof(woof_name));
	strncat(woof_name,wf_name,sizeof(woof_name)-strlen(woof_name));

	wf = WooFOpen(wf_name);
	if(wf == NULL) {
		fprintf(stderr,"WooFPut: couldn't open %s\n",woof_name);
		fflush(stderr);
		return(-1);
	}

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

	ev = EventCreate(TRIGGER,Host_id);
	if(ev == NULL) {
		fprintf(stderr,"WooFPut: couldn't create log event\n");
		exit(1);
	}
	ev->cause_host = Host_id;
	ev->cause_seq_no = my_log_seq_no;
	

	/*
	 * now drop the element in the object
	 */
	P(&wf->mutex);
	/*
	 * find the next spot
	 */
	next = (wf->head + 1) % wf->history_size;
	ndx = next;

	/*
	 * write the data and record the indices
	 */
	buf = (unsigned char *)(MIOAddr(wf->mio)+sizeof(WOOF));
	ptr = buf + (next * (wf->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr+wf->element_size);

	/*
	 * tail is the last valid place where data could go
	 * check to see if it is allocated to a function that
	 * has yet to complete
	 */
	while((el_id->busy == 1) && (next == wf->tail)) {
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

	ev->woofc_ndx = ndx;
	ev->woofc_seq_no = seq_no;
	ev->woofc_element_size = wf->element_size;
	ev->woofc_history_size = wf->history_size;
	memset(ev->woofc_name,0,sizeof(ev->woofc_name));
	strncpy(ev->woofc_name,wf->filename,sizeof(ev->woofc_name));
	memset(ev->woofc_handler,0,sizeof(ev->woofc_handler));
	strncpy(ev->woofc_handler,hand_name,sizeof(ev->woofc_handler));

	/*
	 * log the event so that it can be triggered
	 */
	ls = LogEvent(Host_log,ev);
	if(ls == 0) {
		fprintf(stderr,"WooFPut: couldn't log event\n");
		exit(1);
	}

	EventFree(ev);
	V(&Host_log->tail_wait);
	WooFFree(wf);
	return(1);
}

