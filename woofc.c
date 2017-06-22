#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "woofc.h"


WOOF *WooFCreate(char *name,
	       int (*handler)(WOOF *woof, unsigned long long seq_no, void *el),
	       unsigned long element_size,
	       unsigned long history_size)
{
	WOOF *wf;
	MIO *mio;
	unsigned long space;

	/*
	 * each element gets a seq_no and log index so we can handle
	 * function cancel if we wrap
	 */
	space = ((history_size+1) * (element_size +sizeof(ELID))) + 
			sizeof(WOOF);

	if(name != NULL) {
		mio = MIOOpen(name,"w+",space);
	} else {
		mio = MIOMalloc(space);
	}
	if(mio == NULL) {
		return(NULL);
	}

	wf = (WOOF *)MIOAddr(mio);
	memset(wf,0,sizeof(WOOF));
	wf->mio = mio;

	if(name != NULL) {
		strcpy(wf->filename,name);
	}

	wf->handler = handler;
	wf->history_size = history_size+1;
	wf->element_size = element_size;
	wf->seq_no = 1;

	InitSem(&wf->mutex,1);
	InitSem(&wf->tail_wait,0);

	return(wf);
}

void WooFFree(WOOF *wf)
{
	MIOClose(wf->mio);

	return;
}

struct woof_t_arg
{
	WOOF *wf;
	unsigned long seq_no;
	unsigned long ndx;
};

typedef struct woof_t_arg WOOFARG;

void *WooFThread(void *arg)
{
	WOOFARG *wa = (WOOFARG *)arg;
	WOOF *wf;
	ELID *el_id;
	unsigned char *buf;
	unsigned char *ptr;
	unsigned long ndx;
	unsigned char *el_buf;
	unsigned long seq_no;
	int err;

	wf = (WOOF *)wa->wf;
	buf = (unsigned char *)(MIOAddr(wf->mio)+sizeof(WOOF));
	ndx = wa->ndx;
	ptr = buf + (ndx * (wf->element_size + sizeof(ELID)));

	el_id = (ELID *)(ptr+wf->element_size);

	/*
	 * invoke the function
	 */
	seq_no = wa->seq_no;
	free(wa);
	/* LOGGING
	 * log event start here
	 */
	err = wf->handler(wf,seq_no,(void *)ptr);
	/* LOGGING
	 * log either event success or failure here
	 */
	/*
	 * mark the element as consumed
	 */
	P(&wf->mutex);
	el_id->busy = 0;
	/*
	 * if PUT is waiting for the tail available, signal
	 */
	if(wf->tail == ndx) {
		V(&wf->tail_wait);
	}
	V(&wf->mutex);

	pthread_exit(NULL);
}
		
int WooFPut(WOOF *wf, void *element)
{
	pthread_t tid;
	WOOFARG *wa;
	unsigned long next;
	unsigned char *buf;
	unsigned char *ptr;
	ELID *el_id;
	unsigned long seq_no;
	unsigned long ndx;
	int err;

	P(&wf->mutex);
	/*
	 * find the next spot
	 */
	next = (wf->head + 1) % wf->history_size;


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
	while(next == wf->tail) {
		if(el_id->busy == 1) { /* element in use */
			V(&wf->mutex);
			P(&wf->tail_wait);
			P(&wf->mutex);
			next = (wf->head + 1) % wf->history_size;
		}
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

	/*
	 * now launch the handler
	 */
	wa = (WOOFARG *)malloc(sizeof(WOOFARG));
	if(wa == NULL) {
		/* LOGGING
		 * log malloc failure here
		 */
		return(-1);
	}

	wa->wf = wf;
	wa->seq_no = seq_no;
	wa->ndx = ndx;

	err = pthread_create(&tid,NULL,WooFThread,(void *)wa);
	if(err < 0) {
		free(wa);
		/* LOGGING
		 * log thread create failure here
		 */
		return(-1);
	}
	pthread_detach(tid);

	return(1);
}

	


	
