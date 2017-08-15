#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>


#include "woofc.h"


LOG *Name_log;
unsigned long Name_id;
char WooF_namespace[2048];
char WooF_dir[2048];
char Host_ip[25];
char WooF_namelog_dir[2048];
char Namelog_name[2048];


int main(int argc, char **argv, char **envp)
{
	char *wnld;
	char *wf_ns;
	char *wf_dir;
	char *wf_name;
	char *wf_size;
	char *wf_ndx;
	char *wf_seq_no;
	char *wf_ip;
	char *namelog_name;
	char *namelog_size_str;
	char *namelog_seq_no;
	char *name_id;
	MIO *mio;
	MIO *lmio;
	char full_name[2048];
	char log_name[4096];

	WOOF *wf;
	WOOF_SHARED *wfs;
	ELID *el_id;
	unsigned char *buf;
	unsigned char *ptr;
	unsigned char *farg;
	unsigned long ndx;
	unsigned char *el_buf;
	unsigned long seq_no;
	unsigned long next;
	unsigned long my_log_seq_no; /* needed for logging cause */
	unsigned long namelog_size;
	int err;
	char *st = "woofc_obj2_handler_1";
	int i;

	if(envp != NULL) {
		i = 0;
		while(envp[i] != NULL) {
			putenv(envp[i]);
			i++;
		}
	}

	/*
	 * close stdin to make docker happy
	 */
	fclose(stdin);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: called for handler %s\n",st);
	fflush(stdout);
#endif

	wf_ns = getenv("WOOFC_NAMESPACE");
	if(wf_ns == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOFC_NAMESPACE\n");
		fflush(stderr);
		exit(1);
	}

	strncpy(WooF_namespace,wf_ns,sizeof(WooF_namespace));

#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOFC_NAMESPACE=%s\n",wf_ns);
	fflush(stdout);
#endif

	wf_ip = getenv("WOOF_HOST_IP");
	if(wf_ip == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_HOST_IP\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(Host_ip,wf_ip,sizeof(Host_ip));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_HOST_IP=%s\n",Host_ip);
	fflush(stdout);
#endif

	wf_dir = getenv("WOOFC_DIR");
	if(wf_dir == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOFC_DIR\n");
		fflush(stderr);
		exit(1);
	}

	strncpy(WooF_dir,wf_dir,sizeof(WooF_dir));

#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOFC_DIR=%s\n",wf_dir);
	fflush(stdout);
#endif

	wnld = getenv("WOOF_NAMELOG_DIR");
	if(wnld == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_NAMELOG_DIR\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(WooF_namelog_dir,wnld,sizeof(WooF_namelog_dir));

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
	ndx = (unsigned long)atol(wf_ndx);
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
	seq_no = (unsigned long)atol(wf_seq_no);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_SHEPHERD_SEQ_NO=%lu\n",seq_no);
	fflush(stdout);
#endif
fprintf(stdout,"WooFShepherd: WOOF_SHEPHERD_SEQ_NO=%lu\n",seq_no);
fflush(stdout);

	namelog_name = getenv("WOOF_NAMELOG_NAME");
	if(namelog_name == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_NAMELOG_NAME\n");
		fflush(stderr);
		exit(1);
	}
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_NAMELOG_NAME=%s\n",namelog_name);
	fflush(stdout);
#endif

	strncpy(Namelog_name,namelog_name,sizeof(Namelog_name));

	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if(namelog_seq_no == NULL) {
		fprintf(stderr,
	"WooFShepherd: couldn't find WOOF_NAMELOG_SEQNO for log %s wf %s\n",
			namelog_name,wf_name);
		fflush(stderr);
		exit(1);
	}
	my_log_seq_no = (unsigned long)atol(namelog_seq_no);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_NAMELOG_SEQNO=%lu\n",my_log_seq_no);
	fflush(stdout);
#endif

	name_id = getenv("WOOF_NAME_ID");
	if(name_id == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_NAME_ID\n");
		fflush(stderr);
		exit(1);
	}

	Name_id = (unsigned long)atol(name_id);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: WOOF_NAME_ID=%lu\n",Name_id);
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

#if 0
	mio = MIOReOpen(full_name);
	if(mio == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't open %s with size %lu\n",full_name);
		fflush(stderr);
		exit(1);
	}
#endif

	wf = WooFOpen(wf_name);
	if(wf == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't open %s\n",full_name);
		fflush(stderr);
		exit(1);
	}

	wfs = wf->shared;
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: woof %s open\n",full_name);
	fflush(stdout);
#endif

	sprintf(log_name,"%s/%s",WooF_namelog_dir,namelog_name);
	lmio = MIOReOpen(log_name);
	if(lmio == NULL) {
		fprintf(stderr,
		"WooFShepherd: couldn't open mio for log %s\n",log_name);
		fflush(stderr);
		exit(1);
	}
	Name_log = (LOG *)MIOAddr(lmio);
	strncpy(Namelog_name,namelog_name,sizeof(Namelog_name));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: log %s open\n",namelog_name);
	fflush(stdout);
#endif


#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: wf: 0x%u assigned, el size: %lu\n",
			wfs,wfs->element_size);
	fflush(stdout);
#endif

//	buf = (unsigned char *)(MIOAddr(mio)+sizeof(WOOF));
	buf = (unsigned char *)(((void *)wfs)+sizeof(WOOF_SHARED));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: buf assigned 0x%u\n",buf);
	fflush(stdout);
#endif
	ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: ptr assigned\n");
	fflush(stdout);
#endif
	el_id = (ELID *)(ptr+wfs->element_size);

	farg = (unsigned char *)malloc(wfs->element_size);
	if(farg == NULL) {
		fprintf(stderr,"WooFShepherd: no space for farg of size %d\n",
				wfs->element_size);
		fflush(stderr);
		return(-1);
	}

	memcpy(farg,ptr,wfs->element_size);

	/*
	 * now that we have the argument copied, free the slot in the woof
	 */

#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: about to enter mutex\n");
	fflush(stdout);
#endif
	P(&wfs->mutex);

	/*
	 * mark element as done to prevent handler crash from causing deadlock
	 */
	el_id->busy = 0;
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: marked el done at %lu and signalling\n",ndx);
	fflush(stdout);
#endif

	V(&wfs->tail_wait);

#if 0
	next = (wfs->head + 1) % wfs->history_size;
	/*
	 * if PUT is waiting for the tail available, signal
	 */
	if(next == ndx) {
		V(&wfs->tail_wait);
	}
#endif

	V(&wfs->mutex);
	/*
	 * invoke the function
	 */
	/* LOGGING
	 * log event start here
	 */
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: invoking %s, seq_no: %lu\n",st,seq_no);
	fflush(stdout);
#endif
	err = woofc_obj2_handler_1(wf,seq_no,(void *)farg);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: %s done with seq_no: %lu\n",
		st,seq_no);
	fflush(stdout);
#endif
	free(farg);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: called free, seq_no: %lu\n",seq_no);
	fflush(stdout);
#endif
	/* LOGGING
	 * log either event success or failure here
	 */

#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: calling WooFFree, seq_no: %lu\n",seq_no);
	fflush(stdout);
#endif
fprintf(stdout,"WooFShepherd: calling WooFFree, seq_no: %lu\n",seq_no);
fflush(stdout);
	WooFFree(wf);
	MIOClose(lmio);
#ifdef DEBUG
	fprintf(stdout,"WooFShepherd: exiting, seq_no: %lu\n",seq_no);
	fflush(stdout);
#endif
fprintf(stdout,"WooFShepherd: exiting, seq_no: %lu\n",seq_no);
fflush(stdout);
	exit(0);
	return(0);
}
		

