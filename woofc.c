#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "log.h"
#include "woofc.h"
#include "woofc-access.h"

extern char WooF_namespace[2048];
extern char WooF_dir[2048];
extern char WooF_namelog_dir[2048];
extern char Namelog_name[2048];
extern unsigned long Name_id;
extern LOG *Name_log;

int WooFCreate(char *name,
	       unsigned long element_size,
	       unsigned long history_size)
{
	WOOF_SHARED *wfs;
	MIO *mio;
	unsigned long space;
	char local_name[4096];
	char fname[20];
	int err;

	if(name == NULL) {
		return(-1);
	}


	/*
	 * each element gets a seq_no and log index so we can handle
	 * function cancel if we wrap
	 */
	space = ((history_size+1) * (element_size +sizeof(ELID))) + 
			sizeof(WOOF_SHARED);

	if(WooF_dir == NULL) {
		fprintf(stderr,"WooFCreate: must init system\n");
		fflush(stderr);
		exit(1);
	}

	memset(local_name,0,sizeof(local_name));
	if(WooFValidURI(name)) {
		err = WooFNameSpaceFromURI(name,local_name,sizeof(local_name));
		if(err < 0) {
			fprintf(stderr,"WooFCreate: bad namespace in URI %s\n",
				name);
			fflush(stderr);
			return(-1);
		}
		if(local_name[strlen(local_name)-1] != '/') {
			strncat(local_name,"/",1);
		}
		err = WooFNameFromURI(name,fname,sizeof(fname));
		if(err < 0) {
			fprintf(stderr,"WooFCreate: bad name in URI %s\n",
				name);
			fflush(stderr);
			return(-1);
		}
		strncat(local_name,fname,sizeof(fname));
	} else { /* assume this is WooF_dir local */
		strncpy(local_name,WooF_dir,sizeof(local_name));
		if(local_name[strlen(local_name)-1] != '/') {
			strncat(local_name,"/",1);
		}
		strncat(local_name,name,sizeof(local_name));
	}
		

	mio = MIOOpen(local_name,"w+",space);
	if(mio == NULL) {
		return(-1);
	}
#ifdef DEBUG
	printf("WooFCreate: opened %s\n",local_name);
	fflush(stdout);
#endif

	wfs = (WOOF_SHARED *)MIOAddr(mio);
	memset(wfs,0,sizeof(WOOF_SHARED));

	if(WooFValidURI(name)) {
		strncpy(wfs->filename,fname,sizeof(wfs->filename));
	} else {
		strncpy(wfs->filename,name,sizeof(wfs->filename));
	}

	wfs->history_size = history_size;
	wfs->element_size = element_size;
	wfs->seq_no = 1;

	InitSem(&wfs->mutex,1);
	InitSem(&wfs->tail_wait,history_size);

	MIOClose(mio);

	return(1);
}

WOOF *WooFOpen(char *name)
{
	WOOF *wf;
	WOOF_SHARED *wfs;
	MIO *mio;
	char local_name[4096];
	char fname[4096];
	int err;
	struct stat sbuf;

	if(name == NULL) {
		return(NULL);
	}

	if(WooF_dir[0] == 0) {
		fprintf(stderr,"WooFOpen: must init system\n");
		fflush(stderr);
		exit(1);
	}

	memset(local_name,0,sizeof(local_name));
	strncpy(local_name,WooF_dir,sizeof(local_name));
	if(local_name[strlen(local_name)-1] != '/') {
		strncat(local_name,"/",1);
	}
	if(WooFValidURI(name)) {
		err = WooFNameFromURI(name,fname,sizeof(fname));
		if(err < 0) {
			fprintf(stderr,"WooFCreate: bad name in URI %s\n",
				name);
			fflush(stderr);
			return(NULL);
		}
		strncat(local_name,fname,sizeof(fname));
	} else { /* assume this is WooF_dir local */
		strncat(local_name,name,sizeof(local_name));
	}
#ifdef DEBUG
	printf("WooFOpen: trying to open %s\n",local_name);
	fflush(stdout);
#endif

	if(stat(local_name,&sbuf) < 0) {
		fprintf(stderr,"WooFOpen: couldn't open woof: %s\n",local_name);
		fflush(stderr);
		return(NULL);
	}

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

unsigned long WooFAppend(WOOF *wf, char *hand_name, void *element)
{
	MIO *mio;
	MIO *lmio;
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
	char *namelog_seq_no;
	unsigned long my_log_seq_no;
	EVENT *ev;
	unsigned long ls;
#ifdef DEBUG
	printf("WooFAppend: called %s %s\n",wf->shared->filename,hand_name);
	fflush(stdout);
#endif

	wfs = wf->shared;

	/*
	 * if called from within a handler, env variable carries cause seq_no
	 * for logging
	 *
	 * when called for first time on namespace to start, should be NULL
	 */
	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if(namelog_seq_no != NULL) {
		my_log_seq_no = (unsigned long)atol(namelog_seq_no);
	} else {
		my_log_seq_no = 1;
	}

	if(hand_name != NULL) {
		ev = EventCreate(TRIGGER,Name_id);
		if(ev == NULL) {
			fprintf(stderr,"WooFAppend: couldn't create log event\n");
			exit(1);
		}
		ev->cause_host = Name_id;
		ev->cause_seq_no = my_log_seq_no;
	}

#ifdef DEBUG
	printf("WooFAppend: checking for empty slot\n");
	fflush(stdout);
#endif

	P(&wfs->tail_wait);

#ifdef DEBUG
	printf("WooFAppend: got empty slot\n");
	fflush(stdout);
#endif
	

#ifdef DEBUG
	printf("WooFAppend: adding element\n");
	fflush(stdout);
#endif
	/*
	 * now drop the element in the object
	 */
	P(&wfs->mutex);
#ifdef DEBUG
	printf("WooFAppend: in mutex\n");
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
	printf("WooFAppend: element in\n");
	fflush(stdout);
#endif


#if 0
	while((el_id->busy == 1) && (next == wfs->tail)) {
#ifdef DEBUG
	printf("WooFAppend: busy at %lu\n",next);
	fflush(stdout);
#endif
		V(&wfs->mutex);
		P(&wfs->tail_wait);
		P(&wfs->mutex);
		next = (wfs->head + 1) % wfs->history_size;
		ptr = buf + (next * (wfs->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr+wfs->element_size);
	}
#endif

	/*
	 * write the element
	 */
#ifdef DEBUG
	printf("WooFAppend: writing element 0x%x\n",el_id);
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
	printf("WooFAppend: out of element mutex\n");
	fflush(stdout);
#endif

	/*
	 * if there is no handler, we are done (no need to generate log record)
	 * However, since there is no handler, woofc-shperherd can't V the counting
	 * semaphore for the WooF.  Without a handler, the tail is immediately available since
	 * we don't know whether the WooF will be consumed -- V it in this case
	 */
	if(hand_name == NULL) {
		V(&wfs->tail_wait);
		return(seq_no);
	}

	memset(ev->namespace,0,sizeof(ev->namespace));
	strncpy(ev->namespace,WooF_namespace,sizeof(ev->namespace));
#ifdef DEBUG
	printf("WooFAppend: namespace: %s\n",ev->namespace);
	fflush(stdout);
#endif

	ev->woofc_ndx = ndx;
#ifdef DEBUG
	printf("WooFAppend: ndx: %lu\n",ev->woofc_ndx);
	fflush(stdout);
#endif
	ev->woofc_seq_no = seq_no;
#ifdef DEBUG
	printf("WooFAppend: seq_no: %lu\n",ev->woofc_seq_no);
	fflush(stdout);
#endif
	ev->woofc_element_size = wfs->element_size;
#ifdef DEBUG
	printf("WooFAppend: element_size %lu\n",ev->woofc_element_size);
	fflush(stdout);
#endif
	ev->woofc_history_size = wfs->history_size;
#ifdef DEBUG
	printf("WooFAppend: history_size %lu\n",ev->woofc_history_size);
	fflush(stdout);
#endif
	memset(ev->woofc_name,0,sizeof(ev->woofc_name));
	strncpy(ev->woofc_name,wfs->filename,sizeof(ev->woofc_name));
#ifdef DEBUG
	printf("WooFAppend: name %s\n",ev->woofc_name);
	fflush(stdout);
#endif
	memset(ev->woofc_handler,0,sizeof(ev->woofc_handler));
	strncpy(ev->woofc_handler,hand_name,sizeof(ev->woofc_handler));
#ifdef DEBUG
	printf("WooFAppend: handler %s\n",ev->woofc_handler);
	fflush(stdout);
#endif

	/*
	 * log the event so that it can be triggered
	 */
	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s/%s",WooF_namelog_dir,Namelog_name);
#ifdef DEBUG
	printf("WooFAppend: logging event to %s\n",log_name);
	fflush(stdout);
#endif
	ls = LogEvent(Name_log,ev);
	if(ls == 0) {
		fprintf(stderr,"WooFAppend: couldn't log event to log %s\n",
			log_name);
		fflush(stderr);
		EventFree(ev);
		return(-1);
	}

#ifdef DEBUG
	printf("WooFAppend: logged %lu for woof %s %s\n",
		ls,
		ev->woofc_name,
		ev->woofc_handler);
	fflush(stdout);
#endif

	EventFree(ev);
	V(&Name_log->tail_wait);
	return(seq_no);
}
		
unsigned long WooFPut(char *wf_name, char *hand_name, void *element)
{
	WOOF *wf;
	unsigned long seq_no;
	unsigned long el_size;
	char wf_namespace[2048];
	int err;

#ifdef DEBUG
	printf("WooFPut: called %s %s\n",wf_name,hand_name);
	fflush(stdout);
#endif

	if(WooF_dir[0] == 0) {
		fprintf(stderr,"WooFPut: must init system\n");
		fflush(stderr);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFPut: namespace: %s,  WooF_dir: %s, name: %s\n",
		WooF_namespace,WooF_dir,wf_name);
	fflush(stdout);
#endif

	memset(wf_namespace,0,sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name,wf_namespace,sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote put
	 *
	 * err < 0 implies that name is local name
	 *
	 * for now, assume that the biggest element for remote name space is 10K
	 */
	if((err >= 0) && (strcmp(WooF_namespace,wf_namespace) != 0)) {
		el_size = WooFMsgGetElSize(wf_name);
		if(el_size != (unsigned long)-1) {
			seq_no = WooFMsgPut(wf_name,hand_name,element,el_size);
			return(seq_no);
		} else {
			fprintf(stderr,"WooFPut: couldn't get element size for %s\n",
				wf_name);
			fflush(stderr);
			return(-1);
		}
	}

	wf = WooFOpen(wf_name);

	if(wf == NULL) {
		return(-1);
	}

#ifdef DEBUG
	printf("WooFPut: WooF %s open\n",wf_name);
	fflush(stdout);
#endif
	seq_no = WooFAppend(wf,hand_name,element);

	WooFFree(wf);
	return(seq_no);
}

int WooFGet(char *wf_name, void *element, unsigned long seq_no)
{
	WOOF *wf;
	unsigned long el_size;
	char wf_namespace[2048];
	int err;

#ifdef DEBUG
	printf("WooFGet: called %s %lu\n",wf_name,seq_no);
	fflush(stdout);
#endif

	if(WooF_dir[0] == 0) {
		fprintf(stderr,"WooFGet: must init system\n");
		fflush(stderr);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFGet: namespace: %s,  WooF_dir: %s, name: %s\n",
		WooF_namespace,WooF_dir,wf_name);
	fflush(stdout);
#endif

	memset(wf_namespace,0,sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name,wf_namespace,sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote put
	 *
	 * err < 0 implies that name is local name
	 *
	 * for now, assume that the biggest element for remote name space is 10K
	 */
	if((err >= 0) && (strcmp(WooF_namespace,wf_namespace) != 0)) {
		el_size = WooFMsgGetElSize(wf_name);
		if(el_size != (unsigned long)-1) {
			err = WooFMsgGet(wf_name,element,el_size,seq_no);
			return(err);
		} else {
			fprintf(stderr,"WooFGet: couldn't get element size for %s\n",
				wf_name);
			fflush(stderr);
			return(-1);
		}
	}

	wf = WooFOpen(wf_name);

	if(wf == NULL) {
		return(-1);
	}

#ifdef DEBUG
	printf("WooFGet: WooF %s open\n",wf_name);
	fflush(stdout);
#endif
	err = WooFRead(wf,element,seq_no);

	WooFFree(wf);
	return(err);
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

int WooFRead(WOOF *wf, void *element, unsigned long seq_no)
{
	unsigned char *buf;
	unsigned char *ptr;
	WOOF_SHARED *wfs;
	unsigned long oldest;
	unsigned long youngest;
	unsigned long last_valid;
	unsigned long ndx;
	ELID *el_id;

	wfs = wf->shared;

	buf = (unsigned char *)(((void *)wfs) + sizeof(WOOF_SHARED));

	P(&wfs->mutex);

	ptr = buf + (wfs->head * (wfs->element_size + sizeof(ELID))); 
	el_id = (ELID *)(ptr + wfs->element_size);
	youngest = el_id->seq_no;

	last_valid = wfs->tail;
	ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID))); 
	el_id = (ELID *)(ptr + wfs->element_size);
	oldest = el_id->seq_no;

	if(oldest == 0) { /* haven't wrapped yet */
		last_valid++;
		ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID))); 
		el_id = (ELID *)(ptr + wfs->element_size);
		oldest = el_id->seq_no;
	}

#ifdef DEBUG
	fprintf(stdout,"WooFRead: head: %lu tail: %lu size: %lu last_valid: %lu seq_no: %lu old: %lu young: %lu\n",
		wfs->head,
		wfs->tail,
		wfs->history_size,
		last_valid,
		seq_no,
		oldest,
		youngest);
#endif

	/*
	 * is the seq_no between head and tail ndx?
	 */
	if((seq_no < oldest) || (seq_no > youngest)) {
		V(&wfs->mutex);
		return(-1);
	}

	/*
	 * yes, compute ndx forward from last_valid ndx
	 */
	ndx = (last_valid + (seq_no - oldest)) % wfs->history_size;
#ifdef DEBUG
	fprintf(stdout,"WooFRead: head: %lu tail: %lu size: %lu last_valid: %lu seq_no: %lu old: %lu young: %lu ndx: %lu\n",
		wfs->head,
		wfs->tail,
		wfs->history_size,
		last_valid,
		seq_no,
		oldest,
		youngest,
		ndx);
	fflush(stdout);
#endif
	ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);
#ifdef DEBUG
	fprintf(stdout,"WooFRead: seq_no: %lu, found seq_no: %lu\n",seq_no,el_id->seq_no);
	fflush(stdout);
#endif
	memcpy(element,ptr,wfs->element_size);
	V(&wfs->mutex);
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

unsigned long WooFBack(WOOF *wf, unsigned long ndx, unsigned long elements)
{
	WOOF_SHARED *wfs = wf->shared;
	unsigned long remainder = elements % wfs->history_size;
	unsigned long new;
	unsigned long wrap;

	if(elements == 0) {
		return(wfs->head);
	}

	new = ndx - remainder;

	/*
	 * if we need to wrap around
	 */
	if(new >= wfs->history_size) {
		wrap = remainder - ndx;
		new = wfs->history_size - wrap;
	}

	return(new);
}
		
unsigned long WooFForward(WOOF *wf, unsigned long ndx, unsigned long elements)
{
	unsigned long new = (ndx + elements) % wf->shared->history_size;

	return(new);
}

int WooFInvalid(unsigned long seq_no)
{
	if(seq_no == (unsigned long)-1) {
		return(1);
	} else {
		return(0);
	}
}
