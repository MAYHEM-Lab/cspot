#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "woofc.h"

int main(int argc, char **argv, char **envp)
{
	char *wf_dir;
	char *wf_name;
	char *wf_size;
	char *wf_ndx;
	char *wf_seq_no;
	MIO *mio;
	unsigned long mio_size;
	char full_name[2048];

	WOOF *wf;
	ELID *el_id;
	unsigned char *buf;
	unsigned char *ptr;
	unsigned long ndx;
	unsigned char *el_buf;
	unsigned long seq_no;
	unsigned long next;
	int err;

	wf_dir = getenv(WOOF_SHEPHERD_DIR);
	if(wf_dir == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_DIR\n");
		fflush(stderr);
		exit(1);
	}

	wf_name = getenv(WOOF_SHEPHERD_NAME);
	if(wf_name == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_NAME\n");
		fflush(stderr);
		exit(1);
	}

	wf_size = getenv(WOOF_SHEPHERD_SIZE);
	if(wf_size == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_SIZE for %s\n",wf_name);
		fflush(stderr);
		exit(1);
	}
	mio_size = atol(wf_size);

	wf_ndx = getenv(WOOF_SHEPHERD_NDX);
	if(wf_ndx == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_NDX for %s\n",wf_name);
		fflush(stderr);
		exit(1);
	}
	ndx = atol(wf_ndx);

	wf_seq_no = getenv(WOOF_SHEPHERD_SEQNO);
	if(wf_seq_no == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_SEQNO for %s\n",wf_name);
		fflush(stderr);
		exit(1);
	}
	seq_no = atol(wf_ndx);

	strncpy(full_name,wf_dir,sizeof(full_name));
	if(full_name[strlen(full_name)-1] != '/') {
		strncat(full_name,"/",sizeof(full_name)-strlen(full_name));
	}
	strncpy(full_name,wf_name,sizeof(full_name)-strlen(full_name));

	mio = MIOOpen(full_name,"a+",mio_size);
	if(mio == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't open %s with size %lu\n",full_name,mio_size);
		fflush(stderr);
		exit(1);
	}


	wf = (WOOF *)MIOAddr(mio);

	if(strcmp(wf->handler_name,WOOF_HANDLER_NAME) != 0) {
		fprintf(stderr,"WooFShepherd: passed handler name %s doesn't match compiled name %s\n",
				wf->handler_name,WOOF_HANDLER_NAME);
		fflush(stderr);
		MIOClose(mio);
		exit(1);
	}

	buf = (unsigned char *)(MIOAddr(wf->mio)+sizeof(WOOF));
	ptr = buf + (ndx * (wf->element_size + sizeof(ELID)));

	el_id = (ELID *)(ptr+wf->element_size);

	/*
	 * invoke the function
	 */
	/* LOGGING
	 * log event start here
	 */
	err = WOOF_HANDLER_NAME(wf,seq_no,(void *)ptr);
	/* LOGGING
	 * log either event success or failure here
	 */
	/*
	 * mark the element as consumed
	 */
	P(&wf->mutex);
	el_id->busy = 0;

	next = (wf->head + 1) % wf->history_size;
	/*
	 * if PUT is waiting for the tail available, signal
	 */
	if(next == ndx) {
		V(&wf->tail_wait);
	}
	V(&wf->mutex);

	MIOClose(mio);
	pthread_exit(NULL);
	return(0);
}
		

	


	
