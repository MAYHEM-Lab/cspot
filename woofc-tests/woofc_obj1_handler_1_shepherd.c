#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define DEBUG

#include "woofc.h"

static LOG *Host_log;
static unsigned long Host_id;

int main(int argc, char **argv, char **envp)
{
	char *wf_dir;
	char *wf_name;
	char *wf_size;
	char *wf_ndx;
	char *wf_seq_no;
	char *host_log_name;
	char *host_log_size_str;
	char *host_log_seq_no;
	char *host_id;
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
	unsigned long my_log_seq_no; /* needed for logging cause */
	unsigned long host_log_size;
	int err;
	char *st = "woofc_obj1_handler_1";
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: for handler %s\n",st);
	fflush(stdout);
#endif

	wf_dir = getenv("WOOFC_DIR");
	if(wf_dir == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOFC_DIR\n");
		fflush(stderr);
		exit(1);
	}

#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOFC_DIR=%s\n",wf_dir);
	fflush(stdout);
#endif
	wf_name = getenv("WOOF_SHEPHERD_NAME");
	if(wf_name == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_NAME\n");
		fflush(stderr);
		exit(1);
	}
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_SHEPHERD_NAME=%s\n",wf_name);
	fflush(stdout);
#endif

	wf_ndx = getenv("WOOF_SHEPHERD_NDX");
	if(wf_ndx == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_NDX for %s\n",wf_name);
		fflush(stderr);
		exit(1);
	}
	ndx = atol(wf_ndx);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_SHEPHERD_NDX=%lu\n",ndx);
	fflush(stdout);
#endif

	wf_seq_no = getenv("WOOF_SHEPHERD_SEQNO");
	if(wf_seq_no == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_SHEPHERD_SEQNO for %s\n",wf_name);
		fflush(stderr);
		exit(1);
	}
	seq_no = atol(wf_seq_no);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_SHEPHERD_SEQ_NO=%lu\n",seq_no);
	fflush(stdout);
#endif

	host_log_name = getenv("WOOF_HOST_LOG_NAME");
	if(host_log_name == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_HOST_LOG_NAME\n");
		fflush(stderr);
		exit(1);
	}
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_HOST_LOG_NAME=%s\n",host_log_name);
	fflush(stdout);
#endif

	host_log_size_str = getenv("WOOF_HOST_LOG_SIZE");
	if(host_log_size_str == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_HOST_LOG_SIZE\n");
		fflush(stderr);
		exit(1);
	}
	host_log_size = atol(host_log_size_str);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_HOST_LOG_SIZE=%lu\n",host_log_size);
	fflush(stdout);
#endif

	host_log_seq_no = getenv("WOOF_HOST_LOG_SEQNO");
	if(host_log_seq_no == NULL) {
		fprintf(stderr,
	"WooFShepherd: couldn't find WOOF_HOST_LOG_SEQNO for log %s wf %s\n",
			host_log_name,wf_name);
		fflush(stderr);
		exit(1);
	}
	my_log_seq_no = atol(host_log_seq_no);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_HOST_LOG_SEQNO=%lu\n",my_log_seq_no);
	fflush(stdout);
#endif

	host_id = getenv("WOOF_HOST_ID");
	if(host_id == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_HOST_ID\n");
		fflush(stderr);
		exit(1);
	}

	Host_id = atol(host_id);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_HOST_ID=%lu\n",Host_id);
	fflush(stdout);
#endif


	strncpy(full_name,wf_dir,sizeof(full_name));
	if(full_name[strlen(full_name)-1] != '/') {
		strncat(full_name,"/",sizeof(full_name)-strlen(full_name));
	}
	strncat(full_name,wf_name,sizeof(full_name)-strlen(full_name));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WooF name: %s\n",full_name);
	fflush(stdout);
#endif

	mio = MIOReOpen(full_name);
	if(mio == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't open %s with size %lu\n",full_name,mio_size);
		fflush(stderr);
		exit(1);
	}
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: mio %s open\n",full_name);
	fflush(stdout);
#endif

	Host_log = LogOpen(host_log_name,host_log_size);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: log %s open\n",host_log_name);
	fflush(stdout);
#endif


	wf = (WOOF *)MIOAddr(mio);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: wf: 0x%u assigned, el size: %lu\n",
			wf,wf->element_size);
	fflush(stdout);
#endif

	buf = (unsigned char *)(MIOAddr(mio)+sizeof(WOOF));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: buf assigned 0x%u\n",buf);
	fflush(stdout);
//	fprintf(stdout,"WooFShepherd: ptr will be 0x%u\n",buf + (ndx * (wf->element_size + sizeof(ELID))));
//	fflush(stdout);
#endif
	ptr = buf + (ndx * (wf->element_size + sizeof(ELID)));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: ptr assigned\n");
	fflush(stdout);
#endif

	/*
	 * invoke the function
	 */
	/* LOGGING
	 * log event start here
	 */
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: invoking %s\n",st);
	fflush(stdout);
sleep(1);
#endif
	err = woofc_obj1_handler_1(wf,seq_no,(void *)ptr);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: %s done with %d\n",
		st,err);
	fflush(stdout);
#endif
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
		

	


	
