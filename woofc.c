// #define DEBUG
#define REPAIR

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
#include "woofc-cache.h"
#include "repair.h"

extern char WooF_namespace[2048];
extern char WooF_dir[2048];
extern char WooF_namelog_dir[2048];
extern char Namelog_name[2048];
extern unsigned long Name_id;
extern LOG *Name_log;
extern char Host_ip[25];

WOOF_CACHE *WooF_open_cache;
struct woof_open_cache_stc
{
	WOOF *wf;
	unsigned long ino;
	char local_name[1024];
};
typedef struct woof_open_cache_stc WOOF_OPEN_CACHE_EL;
#define WOOF_OPEN_CACHE_MAX (100)
void WooFDrop(WOOF *wf);

int WooFCreate(char *name,
			   unsigned long element_size,
			   unsigned long history_size)
{
	WOOF_SHARED *wfs;
	MIO *mio;
	unsigned long space;
	char local_name[4096];
	char temp_name[4096];
	char fname[1024];
	char ip_str[25];
	int err;
	int is_local;
	struct stat sbuf;
	struct stat obuf;
	int renamed;
	double r;

	if (name == NULL)
	{
		return (-1);
	}

	/*
	 * each element gets a seq_no and log index so we can handle
	 * function cancel if we wrap
	 */
	space = ((history_size + 1) * (element_size + sizeof(ELID))) +
			sizeof(WOOF_SHARED);

	if (WooF_dir == NULL)
	{
		fprintf(stderr, "WooFCreate: must init system\n");
		fflush(stderr);
		exit(1);
	}

	is_local = 0;
	memset(local_name, 0, sizeof(local_name));
	memset(ip_str, 0, sizeof(ip_str));
	/*
	 * if it is a woof:// spec, check to see if the path matches the namespace
	 *
	 * if it does, it is local => use WooF_dir
	 */
	if (WooFValidURI(name))
	{
		err = WooFNameSpaceFromURI(name, local_name, sizeof(local_name));
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate: bad namespace in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		/*
		 * check to see if there is a host spec
		 */
		err = WooFIPAddrFromURI(name, ip_str, sizeof(ip_str));
		if (err < 0)
		{
			strncpy(ip_str, Host_ip, sizeof(ip_str));
		}
		if ((strcmp(WooF_namespace, local_name) == 0) &&
			(strcmp(Host_ip, ip_str) == 0))
		{
			/*
			 * woof spec for local name space, use WooF_dir
			 */
			is_local = 1;
			memset(local_name, 0, sizeof(local_name));
			strncpy(local_name, WooF_dir, sizeof(local_name));
			if (local_name[strlen(local_name) - 1] != '/')
			{
				strncat(local_name, "/", 1);
			}
			err = WooFNameFromURI(name, fname, sizeof(fname));
			if (err < 0)
			{
				fprintf(stderr, "WooFCreate: bad name in URI %s\n",
						name);
				fflush(stderr);
				return (-1);
			}
			strncat(local_name, fname, sizeof(fname));
		}
	}
	else
	{ /* not URI spec so must be local */
		is_local = 1;
		strncpy(local_name, WooF_dir, sizeof(local_name));
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		strncat(local_name, name, sizeof(local_name));
	}

	if (is_local == 0)
	{
		fprintf(stderr, "WooFCreate: non-local create of %s not supported (yet)\n",
				name);
		fflush(stderr);
		return (-1);
	}

#ifdef NOTRIGHTNOW
	/*
	 * here is where a remote create message would go
	 */
	if (WooFValidURI(name) && (is_local == 0))
	{
		err = WooFNameSpaceFromURI(name, local_name, sizeof(local_name));
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate: bad namespace in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		err = WooFNameFromURI(name, fname, sizeof(fname));
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate: bad name in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		strncat(local_name, fname, sizeof(fname));
	}
	else
	{ /* assume this is WooF_dir local */
		strncpy(local_name, WooF_dir, sizeof(local_name));
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		strncat(local_name, name, sizeof(local_name));
	}
#endif

	/*
	 * here is a HACK for open woof caching.  If the woof being created already exists, rename it so that the new
	 * woof gets a different inode number (guaranteed).  That way, a cached open woof can "tell" (by getting the
	 * inode number from a stat) whether the woof has changed since it was cached
	 */

	renamed = 0;
	if (stat(local_name, &sbuf) >= 0)
	{
		memset(temp_name, 0, sizeof(temp_name));
		r = drand48();
		sprintf(temp_name, "%s.%10.10f", local_name, r);
		rename(local_name, temp_name);
		renamed = 1;
#ifdef DEBUG
		printf("WooFCreate: renamed %s to %s\n", local_name, temp_name);
		fflush(stdout);
#endif
	}

	mio = MIOOpen(local_name, "w+", space);
	if (mio == NULL)
	{
		fprintf(stderr, "WooFCreate: couldn't open %s with space %lu\n", local_name, space);
		fflush(stderr);
		rename(temp_name, local_name);
		return (-1);
	}
#ifdef DEBUG
	if (stat(local_name, &obuf) >= 0)
	{
		printf("WooFCreate: opened %s with inode %lu\n", local_name, obuf.st_ino);
	}
	else
	{
		printf("WooFCreate: opened %s\n", local_name);
	}
	fflush(stdout);
#endif

	if (renamed == 1)
	{
#ifdef DEBUG
		printf("WooFCreate: attempting unlick of %s\n", temp_name);
		fflush(stdout);
#endif
		err = unlink(temp_name);
		if (err < 0)
		{
			fprintf(stderr, "WooFCreate: couldn't dispose of %s\n", temp_name);
			fflush(stderr);
		}
	}

	wfs = (WOOF_SHARED *)MIOAddr(mio);
	memset(wfs, 0, sizeof(WOOF_SHARED));

	if (WooFValidURI(name))
	{
		strncpy(wfs->filename, fname, sizeof(wfs->filename));
	}
	else
	{
		strncpy(wfs->filename, name, sizeof(wfs->filename));
	}

	wfs->history_size = history_size;
	wfs->element_size = element_size;
	wfs->seq_no = 1;
#ifdef REPAIR
	wfs->repair_mode = NORMAL;
#endif

	InitSem(&wfs->mutex, 1);
	InitSem(&wfs->tail_wait, history_size);

	MIOClose(mio);

	return (1);
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
	WOOF_OPEN_CACHE_EL *wel;
	WOOF_OPEN_CACHE_EL *pel;
#ifdef REPAIR
	char shadow_name[4096];
#endif

	if (name == NULL)
	{
		return (NULL);
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFOpen: must init system\n");
		fflush(stderr);
		exit(1);
	}

#ifdef CACHE_ON
	if (WooF_open_cache == NULL)
	{
		WooF_open_cache = WooFCacheInit(WOOF_OPEN_CACHE_MAX);
	}
#endif

	memset(local_name, 0, sizeof(local_name));
	strncpy(local_name, WooF_dir, sizeof(local_name));
	if (local_name[strlen(local_name) - 1] != '/')
	{
		strncat(local_name, "/", 1);
	}
	if (WooFValidURI(name))
	{
		err = WooFNameFromURI(name, fname, sizeof(fname));
		if (err < 0)
		{
			fprintf(stderr, "WooFOpen: bad name in URI %s\n",
					name);
			fflush(stderr);
			return (NULL);
		}
		strncat(local_name, fname, sizeof(fname));
	}
	else
	{ /* assume this is WooF_dir local */
		strncat(local_name, name, sizeof(local_name));
	}
#ifdef DEBUG
	printf("WooFOpen: trying to open %s from fname %s, %s with dir %s\n",
		   local_name,
		   fname,
		   name,
		   WooF_dir);
	fflush(stdout);
#endif

	if (stat(local_name, &sbuf) < 0)
	{
		fprintf(stderr, "WooFOpen: couldn't open woof: %s\n", local_name);
		fflush(stderr);
		return (NULL);
	}

	/*
	 * here is the HACK for open woof caching
	 */
	if (WooF_open_cache != NULL)
	{
		wel = WooFCacheFind(WooF_open_cache, local_name);
		if (wel != NULL)
		{
			/*
			 * if the woof hasn't been recreated
			 * it is still "good"
			 */
			if (wel->ino == sbuf.st_ino)
			{
#ifdef DEBUG
				printf("WooFOpen: found cached woof for %s\n", local_name);
				fflush(stdout);
#endif
				return (wel->wf);
			}
			else
			{
#ifdef DEBUG
				printf("WooFOpen: expiring cached woof for %s\n", local_name);
				fflush(stdout);
#endif
				WooFCacheRemove(WooF_open_cache, local_name);
				WooFDrop(wel->wf);
				free(wel);
			}
		}
	}

	mio = MIOReOpen(local_name);
	if (mio == NULL)
	{
		return (NULL);
	}
#ifdef DEBUG
	printf("WooFOpen: opened %s\n", local_name);
	fflush(stdout);
#endif

	wf = (WOOF *)malloc(sizeof(WOOF));
	if (wf == NULL)
	{
		MIOClose(mio);
		return (NULL);
	}
	memset(wf, 0, sizeof(WOOF));

	wf->shared = (WOOF_SHARED *)MIOAddr(mio);
	wf->mio = mio;
	wf->ino = sbuf.st_ino;

	if (WooF_open_cache != NULL)
	{
		wel = (WOOF_OPEN_CACHE_EL *)malloc(sizeof(WOOF_OPEN_CACHE_EL));
		if ((wel != NULL) && (stat(local_name, &sbuf) >= 0))
		{
			wel->wf = wf;
			wel->ino = sbuf.st_ino;
		}
#ifdef DEBUG
		printf("WooFOpen: inserting cached woof for %s\n", local_name);
		fflush(stdout);
#endif
		err = WooFCacheInsert(WooF_open_cache, local_name, (void *)wel);
		/*
		 * try only once on failure
		 */
		if (err < 0)
		{
			pel = (WOOF_OPEN_CACHE_EL *)WooFCacheAge(WooF_open_cache);
			if (pel != NULL)
			{
				WooFDrop(pel->wf);
				free(pel);
			}
			err = WooFCacheInsert(WooF_open_cache, local_name, (void *)wel);
			if (err < 0)
			{
				free(wel);
			}
		}
	}

#ifdef REPAIR
	if (wf->shared->repair_mode & REPAIRING)
	{
		sprintf(shadow_name, "%s_shadow", name);
		WooFDrop(wf);
#ifdef DEBUG
		printf("WooFOpen: WooF %s is being repaired, open shadow instead\n", name);
		fflush(stdout);
#endif
		wf = WooFOpenOriginal(shadow_name);
	}
#endif
	return (wf);
}

void WooFFree(WOOF *wf)
{
	if (WooF_open_cache == NULL)
	{
		MIOClose(wf->mio);
		free(wf);
	}

	return;
}

void WooFDrop(WOOF *wf)
{
	MIOClose(wf->mio);
	free(wf);
	return;
}

unsigned long WooFAppendWithCause(WOOF *wf, char *hand_name, void *element, unsigned long cause_host, unsigned long long cause_seq_no)
{
	MIO *mio;
	MIO *lmio;
	WOOF_SHARED *wfs;
	char log_name[4096];
	char shadow_name[2048];
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
	printf("WooFAppendWithCause: called %s %s\n", wf->shared->filename, hand_name);
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
	if (namelog_seq_no != NULL)
	{
		my_log_seq_no = strtoul(namelog_seq_no, (char **)NULL, 10);
	}
	else
	{
		my_log_seq_no = 0;
	}

	if (hand_name != NULL)
	{
		ev = EventCreate(TRIGGER, Name_id);
	}
	else
	{
		ev = EventCreate(APPEND, Name_id);
	}
	if (ev == NULL)
	{
		fprintf(stderr, "WooFAppendWithCause: couldn't create log event\n");
		exit(1);
	}
	EventSetCause(ev, cause_host, cause_seq_no);

#ifdef DEBUG
	printf("WooFAppendWithCause: checking for empty slot, ino: %lu\n", wf->ino);
	fflush(stdout);
#endif

	P(&wfs->tail_wait);

#ifdef DEBUG
	printf("WooFAppendWithCause: got empty slot\n");
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFAppendWithCause: adding element\n");
	fflush(stdout);
#endif
	/*
	 * now drop the element in the object
	 */
	P(&wfs->mutex);
#ifdef DEBUG
	printf("WooFAppendWithCause: in mutex\n");
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
	printf("WooFAppendWithCause: element in\n");
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
	printf("WooFAppendWithCause: writing element 0x%x\n", el_id);
	fflush(stdout);
#endif
	memcpy(ptr, element, wfs->element_size);
	/*
	 * and elemant meta data after it
	 */
	el_id->seq_no = wfs->seq_no;
	el_id->busy = 1;

	/*
	printf("WooFAppendWithCause: about to set head for name %s, head: %lu, next: %lu, size: %lu\n",
	wfs->filename,wfs->head,next,wfs->history_size);
	fflush(stdout);
	*/

	/*
	 * update circular buffer
	 */
	ndx = wfs->head = next;
	if (next == wfs->tail)
	{
		wfs->tail = (wfs->tail + 1) % wfs->history_size;
	}
	seq_no = wfs->seq_no;
	wfs->seq_no++;

#ifdef REPAIR
	/*
	 * forward the shadow woof
	 */
	if (wfs->repair_mode & SHADOW)
	{
		err = WooFShadowForward(wf);
		if (err < 0)
		{
			fprintf(stderr, "WooFAppendWithCause: couldn't forward shadow %s\n", wfs->filename);
			fflush(stderr);
			return (-1);
		}
		if (wfs->repair_mode == NORMAL) // shadow closed
		{
			// TODO: delete shadow
			wfs->repair_mode = REMOVE;
		}
	}
#endif

	V(&wfs->mutex);
#ifdef DEBUG
	printf("WooFAppendWithCause: out of element mutex\n");
	fflush(stdout);
#endif

	memset(ev->namespace, 0, sizeof(ev->namespace));
	strncpy(ev->namespace, WooF_namespace, sizeof(ev->namespace));
#ifdef DEBUG
	printf("WooFAppendWithCause: namespace: %s\n", ev->namespace);
	fflush(stdout);
#endif

	ev->woofc_ndx = ndx;
#ifdef DEBUG
	printf("WooFAppendWithCause: ndx: %lu\n", ev->woofc_ndx);
	fflush(stdout);
#endif
	ev->woofc_seq_no = seq_no;
#ifdef DEBUG
	printf("WooFAppendWithCause: seq_no: %lu\n", ev->woofc_seq_no);
	fflush(stdout);
#endif
	ev->woofc_element_size = wfs->element_size;
#ifdef DEBUG
	printf("WooFAppendWithCause: element_size %lu\n", ev->woofc_element_size);
	fflush(stdout);
#endif
	ev->woofc_history_size = wfs->history_size;
#ifdef DEBUG
	printf("WooFAppendWithCause: history_size %lu\n", ev->woofc_history_size);
	fflush(stdout);
#endif
	memset(ev->woofc_name, 0, sizeof(ev->woofc_name));
	strncpy(ev->woofc_name, wfs->filename, sizeof(ev->woofc_name));
#ifdef DEBUG
	printf("WooFAppendWithCause: name %s\n", ev->woofc_name);
	fflush(stdout);
#endif

	if (hand_name != NULL)
	{
		memset(ev->woofc_handler, 0, sizeof(ev->woofc_handler));
		strncpy(ev->woofc_handler, hand_name, sizeof(ev->woofc_handler));
#ifdef DEBUG
		printf("WooFAppendWithCause: handler %s\n", ev->woofc_handler);
		fflush(stdout);
#endif
	}

	ev->ino = wf->ino;
#ifdef DEBUG
	printf("WooFAppendWithCause: ino %lu\n", ev->ino);
	fflush(stdout);
#endif

	/*
	 * log the event so that it can be triggered
	 */
	memset(log_name, 0, sizeof(log_name));
	sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);
#ifdef DEBUG
	printf("WooFAppendWithCause: logging event to %s\n", log_name);
	fflush(stdout);
#endif

	ls = LogEvent(Name_log, ev);
	if (ls == 0)
	{
		fprintf(stderr, "WooFAppendWithCause: couldn't log event to log %s\n",
				log_name);
		fflush(stderr);
		EventFree(ev);
		if (hand_name == NULL)
		{
			/*
			 * mark the woof as done for purposes of sync
			 */
#if DONEFLAG
			wfs->done = 1;
#endif
			V(&wfs->tail_wait);
			return (-1);
		}
	}

#ifdef DEBUG
	printf("WooFAppendWithCause: logged %lu for woof %s %s\n",
		   ls,
		   ev->woofc_name,
		   ev->woofc_handler);
	fflush(stdout);
#endif

	EventFree(ev);

	if (hand_name == NULL)
	{
		/*
		 * mark the woof as done for purposes of sync
		 */
#if DONEFLAG
		wfs->done = 1;
#endif
		V(&wfs->tail_wait);
	}
	else
	{
		V(&Name_log->tail_wait);
	}
	return (seq_no);
}

unsigned long WooFPut(char *wf_name, char *hand_name, void *element)
{
	WOOF *wf;
	unsigned long seq_no;
	unsigned long el_size;
	char wf_namespace[2048];
	char shadow_name[2048];
	char ns_ip[25];
	char my_ip[25];
	int err;
	char *namelog_seq_no;
	unsigned long my_log_seq_no;
	clock_t start;
	int diff;

#ifdef DEBUG
	printf("WooFPut: called %s %s\n", wf_name, hand_name);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	/*
	 * if there is no IP address in the URI, use the local IP address
	 */
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFPut: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFPut: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote put
	 *
	 * err < 0 implies that name is local name
	 *
	 * if namespace paths do not match or they do match but the IP addresses do not match, this is
	 * a remote put
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		el_size = WooFMsgGetElSize(wf_name);
		if (el_size != (unsigned long)-1)
		{
			seq_no = WooFMsgPut(wf_name, hand_name, element, el_size);
			return (seq_no);
		}
		else
		{
			fprintf(stderr, "WooFPut: couldn't get element size for %s\n",
					wf_name);
			fflush(stderr);
			return (-1);
		}
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFPut: local namespace put must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFPut: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, wf_name);
	fflush(stdout);
#endif
	wf = WooFOpen(wf_name);

	if (wf == NULL)
	{
		return (-1);
	}

#ifdef DEBUG
	printf("WooFPut: WooF %s open\n", wf_name);
	fflush(stdout);
#endif
	// seq_no = WooFAppend(wf, hand_name, element);
	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if (namelog_seq_no != NULL)
	{
		my_log_seq_no = strtoul(namelog_seq_no, (char **)NULL, 10);
	}
	else
	{
		my_log_seq_no = 0;
	}
	seq_no = WooFAppendWithCause(wf, hand_name, element, Name_id, my_log_seq_no);
	if (wf->shared->repair_mode == REMOVE)
	{
		memset(shadow_name, 0, sizeof(shadow_name));
		strncpy(shadow_name, WooF_dir, sizeof(shadow_name));
		if (shadow_name[strlen(shadow_name) - 1] != '/')
		{
			strncat(shadow_name, "/", 1);
		}
		strncat(shadow_name, wf_name, sizeof(shadow_name));
		strncat(shadow_name, "_shadow", 7);
#ifdef DEBUG
		printf("WooFPut: deleting shadow file %s\n", shadow_name);
		fflush(stdout);
#endif
		WooFFree(wf);
		err = remove(shadow_name);
		if (err < 0)
		{
			fprintf(stderr, "WooFPut: couldn't delete shadow file %s\n", shadow_name);
			fflush(stderr);
		}
		printf("WooFPut: deleted shadow file %s\n", shadow_name);
		fflush(stdout);
		return (seq_no);
	}
	WooFFree(wf);
	return (seq_no);
}

unsigned long WooFPutWithCause(char *wf_name, char *hand_name, void *element, unsigned long cause_host, unsigned long long cause_seq_no)
{
	WOOF *wf;
	unsigned long seq_no;
	unsigned long el_size;
	char wf_namespace[2048];
	char shadow_name[2048];
	char ns_ip[25];
	char my_ip[25];
	int err;

#ifdef DEBUG
	printf("WooFPutWithCause: called %s %s\n", wf_name, hand_name);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	/*
	 * if there is no IP address in the URI, use the local IP address
	 */
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFPut: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFPutWithCause: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote put
	 *
	 * err < 0 implies that name is local name
	 *
	 * if namespace paths do not match or they do match but the IP addresses do not match, this is
	 * a remote put
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		el_size = WooFMsgGetElSize(wf_name);
		if (el_size != (unsigned long)-1)
		{
			seq_no = WooFMsgPut(wf_name, hand_name, element, el_size);
			return (seq_no);
		}
		else
		{
			fprintf(stderr, "WooFPutWithCause: couldn't get element size for %s\n",
					wf_name);
			fflush(stderr);
			return (-1);
		}
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFPutWithCause: local namespace put must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFPutWithCause: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, wf_name);
	fflush(stdout);
#endif
	wf = WooFOpen(wf_name);

	if (wf == NULL)
	{
		return (-1);
	}

#ifdef DEBUG
	printf("WooFPutWithCause: WooF %s open\n", wf_name);
	fflush(stdout);
#endif
	seq_no = WooFAppendWithCause(wf, hand_name, element, cause_host, cause_seq_no);
	if (wf->shared->repair_mode == REMOVE)
	{
		memset(shadow_name, 0, sizeof(shadow_name));
		strncpy(shadow_name, WooF_dir, sizeof(shadow_name));
		if (shadow_name[strlen(shadow_name) - 1] != '/')
		{
			strncat(shadow_name, "/", 1);
		}
		strncat(shadow_name, wf_name, sizeof(shadow_name));
		strncat(shadow_name, "_shadow", 7);
#ifdef DEBUG
		printf("WooFPutWithCause: deleting shadow file %s\n", shadow_name);
		fflush(stdout);
#endif
		WooFFree(wf);
		err = remove(shadow_name);
		if (err < 0)
		{
			fprintf(stderr, "WooFPutWithCause: couldn't delete shadow file %s\n", shadow_name);
			fflush(stderr);
		}
		return (seq_no);
	}
	WooFFree(wf);
	return (seq_no);
}

int WooFGet(char *wf_name, void *element, unsigned long seq_no)
{
	WOOF *wf;
	unsigned long el_size;
	char wf_namespace[2048];
	int err;
	char ns_ip[25];
	char my_ip[25];
	char *namelog_seq_no;
	unsigned long my_log_seq_no;

#ifdef DEBUG
	printf("WooFGet: called %s %lu\n", wf_name, seq_no);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFGet: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFGet: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote get
	 *
	 * err < 0 implies that name is local name
	 *
	 * if the name space paths do not match or they do but the IP addresses don't this
	 * is remote
	 *
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		el_size = WooFMsgGetElSize(wf_name);
		if (el_size != (unsigned long)-1)
		{
			err = WooFMsgGet(wf_name, element, el_size, seq_no);
			return (err);
		}
		else
		{
			fprintf(stderr, "WooFGet: couldn't get element size for %s\n",
					wf_name);
			fflush(stderr);
			return (-1);
		}
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFGet: must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFGet: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, wf_name);
	fflush(stdout);
#endif
	wf = WooFOpen(wf_name);

	if (wf == NULL)
	{
		return (-1);
	}

#ifdef DEBUG
	printf("WooFGet: WooF %s open\n", wf_name);
	fflush(stdout);
#endif
	// err = WooFRead(wf, element, seq_no);
	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if (namelog_seq_no != NULL)
	{
		my_log_seq_no = strtoul(namelog_seq_no, (char **)NULL, 10);
	}
	else
	{
		my_log_seq_no = 0;
	}
	err = WooFReadWithCause(wf, element, seq_no, Name_id, my_log_seq_no);

	WooFFree(wf);
	return (err);
}

#if DONEFLAG
int WooFHandlerDone(char *wf_name, unsigned long seq_no)
{
	WOOF *wf;
	unsigned long el_size;
	char wf_namespace[2048];
	int err;
	char ns_ip[25];
	char my_ip[25];
	int retval;

#ifdef DEBUG
	printf("WooFHandlerDone: called %s %lu\n", wf_name, seq_no);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFGet: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFGet: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote get
	 *
	 * err < 0 implies that name is local name
	 *
	 * if the name space paths do not match or they do but the IP addresses don't this
	 * is remote
	 *
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		retval = WooFMsgGetDone(wf_name, seq_no);
		return (retval);
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFHandlerDone: must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFHandlerDone: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, wf_name);
	fflush(stdout);
#endif
	wf = WooFOpen(wf_name);

	if (wf == NULL)
	{
		return (-1);
	}

#ifdef DEBUG
	printf("WooFHandlerDone: WooF %s open\n", wf_name);
	fflush(stdout);
#endif
	if (wf->shared->done == 1)
	{
		retval = 1;
	}
	else
	{
		retval = 0;
	}

	WooFFree(wf);
	return (retval);
}

#endif

unsigned long WooFGetLatestSeqno(char *wf_name)
{
	WOOF *wf;
	char *cause_woof_name;
	char *woof_shepherd_seq_no;
	unsigned long cause_woof_seq_no;

	cause_woof_name = getenv("WOOF_SHEPHERD_NAME");
	woof_shepherd_seq_no = getenv("WOOF_SHEPHERD_SEQNO");
	if (woof_shepherd_seq_no != NULL)
	{
		cause_woof_seq_no = strtoul(woof_shepherd_seq_no, (char **)NULL, 10);
	}
	else
	{
		cause_woof_seq_no = 0;
	}
	return WooFGetLatestSeqnoWithCause(wf_name, cause_woof_name, cause_woof_seq_no);
}

unsigned long WooFGetLatestSeqnoWithCause(char *wf_name, char *cause_woof_name, unsigned long cause_woof_latest_seq_no)
{
	WOOF *wf;
	unsigned long latest_seq_no;
	char wf_namespace[2048];
	int err;
	char ns_ip[25];
	char my_ip[25];
	char log_name[4096];

#ifdef DEBUG
	printf("WooFGetLatestSeqno: called %s\n", wf_name);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFGetLatestSeqno: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFGetLatestSeqno: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote get
	 *
	 * err < 0 implies that name is local name
	 *
	 * if the name space paths do not match or they do but the IP addresses don't this
	 * is remote
	 *
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		latest_seq_no = WooFMsgGetLatestSeqno(wf_name, cause_woof_name, cause_woof_latest_seq_no);
		return (latest_seq_no);
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFGetLatestSeqno: must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFGetLatestSeqno: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, wf_name);
	fflush(stdout);
#endif
	wf = WooFOpen(wf_name);
	if (wf == NULL)
	{
		return (-1);
	}
#ifdef DEBUG
	printf("WooFGetLatestSeqno: WooF %s open\n", wf_name);
	fflush(stdout);
#endif
	latest_seq_no = WooFLatestSeqnoWithCause(wf, Name_id, cause_woof_name, cause_woof_latest_seq_no);
	WooFFree(wf);

	return (latest_seq_no);
}

int WooFReadTail(WOOF *wf, void *elements, int element_count)
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
	while (i < element_count)
	{
		ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
		memcpy(lp, ptr, wfs->element_size);
		lp += wfs->element_size;
		i++;
		ndx = ndx - 1;
		if (ndx >= wfs->history_size)
		{
			ndx = 0;
		}
		if (ndx == wfs->tail)
		{
			break;
		}
	}
	V(&wfs->mutex);

	return (i);
}

unsigned long WooFGetTail(char *wf_name, void *elements, unsigned long element_count)
{
	WOOF *wf;
	unsigned long el_size;
	char wf_namespace[2048];
	int err;
	char ns_ip[25];
	char my_ip[25];

#ifdef DEBUG
	printf("WooFGetTail: called %s\n", wf_name);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFGetTail: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFGetTail: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote get
	 *
	 * err < 0 implies that name is local name
	 *
	 * if the name space paths do not match or they do but the IP addresses don't this
	 * is remote
	 *
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		el_size = WooFMsgGetElSize(wf_name);
		if (el_size != (unsigned long)-1)
		{
			err = WooFMsgGetTail(wf_name, elements, el_size, element_count);
			return (err);
		}
		else
		{
			fprintf(stderr, "WooFGetTail: couldn't get element size for %s\n",
					wf_name);
			fflush(stderr);
			return (-1);
		}
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFGetTail: must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFGetTail: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, wf_name);
	fflush(stdout);
#endif
	wf = WooFOpen(wf_name);

	if (wf == NULL)
	{
		return (-1);
	}

#ifdef DEBUG
	printf("WooFGetTail: WooF %s open\n", wf_name);
	fflush(stdout);
#endif
	err = WooFReadTail(wf, elements, element_count);

	WooFFree(wf);
	return (err);
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

	if ((seq_no == 0) || WooFInvalid(seq_no))
	{
		return (-1);
	}

	wfs = wf->shared;

	buf = (unsigned char *)(((void *)wfs) + sizeof(WOOF_SHARED));

	P(&wfs->mutex);

	ptr = buf + (wfs->head * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);
	youngest = el_id->seq_no;

	if (youngest == 0)
	{ /* nothing in the woof */
		V(&wfs->mutex);
		fprintf(stderr, "WooFRead: there is nothing in the WooF %s yet\n", wfs->filename);
		fflush(stderr);
		return (-1);
	}

	last_valid = wfs->tail;
	ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);
	oldest = el_id->seq_no;

	if (oldest == 0)
	{ /* haven't wrapped yet */
		last_valid++;
		ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr + wfs->element_size);
		oldest = el_id->seq_no;
	}

#ifdef DEBUG
	fprintf(stdout, "WooFRead: head: %lu tail: %lu size: %lu last_valid: %lu seq_no: %lu old: %lu young: %lu\n",
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
	if ((seq_no < oldest) || (seq_no > youngest))
	{
		V(&wfs->mutex);
		fprintf(stdout, "WooFRead: seq_no not in range: seq_no: %lu, oldest: %lu, youngest: %lu\n",
				seq_no, oldest, youngest);
		fflush(stdout);
		return (-1);
	}

	/*
	 * yes, compute ndx forward from last_valid ndx
	 */
	ndx = (last_valid + (seq_no - oldest)) % wfs->history_size;
#ifdef DEBUG
	fprintf(stdout, "WooFRead: head: %lu tail: %lu size: %lu last_valid: %lu seq_no: %lu old: %lu young: %lu ndx: %lu\n",
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
	fprintf(stdout, "WooFRead: seq_no: %lu, found seq_no: %lu\n", seq_no, el_id->seq_no);
	fflush(stdout);
#endif
	memcpy(element, ptr, wfs->element_size);
	V(&wfs->mutex);
	return (1);
}

int WooFReadWithCause(WOOF *wf, void *element, unsigned long seq_no, unsigned long cause_host, unsigned long cause_seq_no)
{
	int err;
	unsigned char *buf;
	unsigned char *ptr;
	WOOF_SHARED *wfs;
	unsigned long oldest;
	unsigned long youngest;
	unsigned long last_valid;
	unsigned long ndx;
	ELID *el_id;
	char log_name[4096];
	EVENT *ev;
	unsigned long ls;

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

	if (oldest == 0)
	{ /* haven't wrapped yet */
		last_valid++;
		ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr + wfs->element_size);
		oldest = el_id->seq_no;
	}

#ifdef DEBUG
	fprintf(stdout, "WooFReadWithCause: head: %lu tail: %lu size: %lu last_valid: %lu seq_no: %lu old: %lu young: %lu\n",
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
	if ((seq_no < oldest) || (seq_no > youngest))
	{
		V(&wfs->mutex);
		fprintf(stdout, "WooFReadWithCause: seq_no not in range: seq_no: %lu, oldest: %lu, youngest: %lu\n",
				seq_no, oldest, youngest);
		fflush(stdout);
		return (-1);
	}

	/*
	 * yes, compute ndx forward from last_valid ndx
	 */
	ndx = (last_valid + (seq_no - oldest)) % wfs->history_size;
#ifdef DEBUG
	fprintf(stdout, "WooFReadWithCause: head: %lu tail: %lu size: %lu last_valid: %lu seq_no: %lu old: %lu young: %lu ndx: %lu\n",
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
	fprintf(stdout, "WooFReadWithCause: seq_no: %lu, found seq_no: %lu\n", seq_no, el_id->seq_no);
	fflush(stdout);
#endif
	memcpy(element, ptr, wfs->element_size);
	V(&wfs->mutex);

	ev = EventCreate(READ, Name_id);
	EventSetCause(ev, cause_host, cause_seq_no);

	memset(ev->namespace, 0, sizeof(ev->namespace));
	strncpy(ev->namespace, WooF_namespace, sizeof(ev->namespace));
#ifdef DEBUG
	printf("WooFReadWithCause: namespace: %s\n", ev->namespace);
	fflush(stdout);
#endif

	ev->woofc_ndx = ndx;
#ifdef DEBUG
	printf("WooFReadWithCause: ndx: %lu\n", ev->woofc_ndx);
	fflush(stdout);
#endif
	ev->woofc_seq_no = seq_no;
#ifdef DEBUG
	printf("WooFReadWithCause: seq_no: %lu\n", ev->woofc_seq_no);
	fflush(stdout);
#endif
	ev->woofc_element_size = wfs->element_size;
#ifdef DEBUG
	printf("WooFReadWithCause: element_size %lu\n", ev->woofc_element_size);
	fflush(stdout);
#endif
	ev->woofc_history_size = wfs->history_size;
#ifdef DEBUG
	printf("WooFReadWithCause: history_size %lu\n", ev->woofc_history_size);
	fflush(stdout);
#endif
	memset(ev->woofc_name, 0, sizeof(ev->woofc_name));
	strncpy(ev->woofc_name, wfs->filename, sizeof(ev->woofc_name));
#ifdef DEBUG
	printf("WooFReadWithCause: name %s\n", ev->woofc_name);
	fflush(stdout);
#endif

	ev->ino = wf->ino;
#ifdef DEBUG
	printf("WooFReadWithCause: ino %lu\n", ev->ino);
	fflush(stdout);
#endif

	/*
	 * log the event so that it can be triggered
	 */
	memset(log_name, 0, sizeof(log_name));
	sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);
#ifdef DEBUG
	printf("WooFReadWithCause: logging event to %s\n", log_name);
	fflush(stdout);
#endif

	ls = LogEvent(Name_log, ev);
	if (ls == 0)
	{
		fprintf(stderr, "WooFReadWithCause: couldn't log event to log %s\n",
				log_name);
		fflush(stderr);
		EventFree(ev);
	}

#ifdef DEBUG
	printf("WooFReadWithCause: logged %lu for woof %s %s\n",
		   ls,
		   ev->woofc_name,
		   ev->woofc_handler);
	fflush(stdout);
#endif

	EventFree(ev);
	return (1);
}

unsigned long WooFEarliest(WOOF *wf)
{
	unsigned long earliest;
	WOOF_SHARED *wfs;

	wfs = wf->shared;

	earliest = (wfs->tail + 1) % wfs->history_size;
	return (earliest);
}

unsigned long WooFLatest(WOOF *wf)
{
	return (wf->shared->head);
}

unsigned long WooFBack(WOOF *wf, unsigned long ndx, unsigned long elements)
{
	WOOF_SHARED *wfs = wf->shared;
	unsigned long remainder = elements % wfs->history_size;
	unsigned long new;
	unsigned long wrap;

	if (elements == 0)
	{
		return (wfs->head);
	}

	new = ndx - remainder;

	/*
	 * if we need to wrap around
	 */
	if (new >= wfs->history_size)
	{
		wrap = remainder - ndx;
		new = wfs->history_size - wrap;
	}

	return (new);
}

unsigned long WooFForward(WOOF *wf, unsigned long ndx, unsigned long elements)
{
	unsigned long new = (ndx + elements) % wf->shared->history_size;

	return (new);
}

int WooFInvalid(unsigned long seq_no)
{
	if (seq_no == (unsigned long)-1)
	{
		return (1);
	}
	else
	{
		return (0);
	}
}

unsigned long WooFLatestSeqno(WOOF *wf)
{
	unsigned long seq_no;

	seq_no = wf->shared->seq_no;

	return (seq_no - 1);
}

unsigned long WooFLatestSeqnoWithCause(WOOF *wf, unsigned long cause_host, char *cause_woof_name, unsigned long cause_woof_latest_seq_no)
{
	WOOF_SHARED *wfs;
	unsigned long latest_seq_no;
	EVENT *ev;
	unsigned long ls;
	char *namelog_seq_no;
	unsigned long my_log_seq_no;
	char log_name[4096];
	MIO *mio;
	char *key;
	char fname[4096];
	char local_name[4096];
	int mapping_count;
	unsigned long *seqno_mapping;
	int i;
	struct stat sbuf;

	wfs = wf->shared;
	latest_seq_no = wf->shared->seq_no - 1;

	key = KeyFromWooF(cause_host, cause_woof_name);
	sprintf(fname, "%s.%s", wfs->filename, key);
	memset(local_name, 0, sizeof(local_name));
	strncpy(local_name, WooF_dir, sizeof(local_name));
	if (local_name[strlen(local_name) - 1] != '/')
	{
		strncat(local_name, "/", 1);
	}
	strncat(local_name, fname, sizeof(fname));
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: trying to open shadow mapping %s\n", local_name);
	fflush(stdout);
#endif
	mio = MIOReOpen(local_name);
	if (mio != NULL)
	{
#ifdef DEBUG
		printf("WooFLatestSeqnoWithCause: shadow mapping %s exists\n", local_name);
		fflush(stdout);
#endif
		seqno_mapping = (unsigned long *)MIOAddr(mio);
		mapping_count = seqno_mapping[0];
		if (cause_woof_latest_seq_no > seqno_mapping[1 + 2 * (mapping_count - 1)])
		{
#ifdef DEBUG
			printf("WooFLatestSeqnoWithCause: cause_woof_latest_seq_no %lu is greater than latest attemp, closing, return %lu\n",
				   cause_woof_latest_seq_no, latest_seq_no);
			fflush(stdout);
#endif
			// TODO: delete shadow mapping file
		}
		else
		{
			int last;
			for (i = 0; i < mapping_count; i++)
			{
				if (cause_woof_latest_seq_no >= seqno_mapping[1 + 2 * i])
				{
					last = i;
					latest_seq_no = seqno_mapping[1 + 2 * i + 1];
				}
				else
				{
#ifdef DEBUG
					printf("WooFLatestSeqnoWithCause: found cause_woof_latest_seq_no %lu, return latest seqno then\n",
						   cause_woof_latest_seq_no);
					fflush(stdout);
#endif
					break;
				}
			}
			if (i >= mapping_count)
			{
#ifdef DEBUG
				printf("WooFLatestSeqnoWithCause: found cause_woof_latest_seq_no %lu, return latest seqno then\n",
					   cause_woof_latest_seq_no);
				fflush(stdout);
#endif
			}
		}
	}
#ifdef DEBUG
	else
	{
		printf("WooFLatestSeqnoWithCause: couldn't open shadow mapping %s\n", local_name);
		fflush(stdout);
	}
#endif

	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if (namelog_seq_no != NULL)
	{
		my_log_seq_no = strtoul(namelog_seq_no, (char **)NULL, 10);
	}
	else
	{
		my_log_seq_no = 0;
	}

	ev = EventCreate(LATEST_SEQNO, Name_id);
	EventSetCause(ev, Name_id, my_log_seq_no);
	memset(ev->namespace, 0, sizeof(ev->namespace));
	strncpy(ev->namespace, WooF_namespace, sizeof(ev->namespace));
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: namespace: %s\n", ev->namespace);
	fflush(stdout);
#endif
	ev->woofc_seq_no = latest_seq_no;
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: latest_seq_no: %lu\n", ev->woofc_seq_no);
	fflush(stdout);
#endif
	ev->woofc_element_size = wfs->element_size;
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: element_size %lu\n", ev->woofc_element_size);
	fflush(stdout);
#endif
	ev->woofc_history_size = wfs->history_size;
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: history_size %lu\n", ev->woofc_history_size);
	fflush(stdout);
#endif
	memset(ev->woofc_name, 0, sizeof(ev->woofc_name));
	strncpy(ev->woofc_name, wfs->filename, sizeof(ev->woofc_name));
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: name %s\n", ev->woofc_name);
	fflush(stdout);
#endif
	ev->ino = wf->ino;
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: ino %lu\n", ev->ino);
	fflush(stdout);
#endif
	/*
	 * use ev->woofc_handler as cause_woof_name
	 * and ev->woofc_ndx as cause_woof_latest_seq_no
	 */
	memset(ev->woofc_handler, 0, sizeof(ev->woofc_handler));
	if (cause_woof_name != NULL)
	{
		strncpy(ev->woofc_handler, cause_woof_name, sizeof(ev->woofc_handler));
	}
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: cause_woof_name %s\n", ev->woofc_handler);
	fflush(stdout);
#endif
	ev->woofc_ndx = cause_woof_latest_seq_no;
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: cause_woof_latest_seq_no: %lu\n", ev->woofc_ndx);
	fflush(stdout);
#endif

	/*
	 * log the event so that it can be triggered
	 */
	memset(log_name, 0, sizeof(log_name));
	sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);
#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: logging event to %s\n", log_name);
	fflush(stdout);
#endif

	ls = LogEvent(Name_log, ev);
	if (ls == 0)
	{
		fprintf(stderr, "WooFLatestSeqnoWithCause: couldn't log event to log %s\n",
				log_name);
		fflush(stderr);
		EventFree(ev);
	}

#ifdef DEBUG
	printf("WooFLatestSeqnoWithCause: logged %lu for woof %s\n",
		   ls, ev->woofc_name);
	fflush(stdout);
#endif
	EventFree(ev);

	return (latest_seq_no);
}

#ifdef REPAIR
WOOF *WooFOpenOriginal(char *name)
{
	WOOF *wf;
	WOOF_SHARED *wfs;
	MIO *mio;
	char local_name[4096];
	char fname[4096];
	int err;
	struct stat sbuf;
	WOOF_OPEN_CACHE_EL *wel;
	WOOF_OPEN_CACHE_EL *pel;

	if (name == NULL)
	{
		return (NULL);
	}

	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFOpenOriginal: must init system\n");
		fflush(stderr);
		exit(1);
	}

#ifdef CACHE_ON
	if (WooF_open_cache == NULL)
	{
		WooF_open_cache = WooFCacheInit(WOOF_OPEN_CACHE_MAX);
	}
#endif

	memset(local_name, 0, sizeof(local_name));
	strncpy(local_name, WooF_dir, sizeof(local_name));
	if (local_name[strlen(local_name) - 1] != '/')
	{
		strncat(local_name, "/", 1);
	}
	if (WooFValidURI(name))
	{
		err = WooFNameFromURI(name, fname, sizeof(fname));
		if (err < 0)
		{
			fprintf(stderr, "WooFOpenOriginal: bad name in URI %s\n",
					name);
			fflush(stderr);
			return (NULL);
		}
		strncat(local_name, fname, sizeof(fname));
	}
	else
	{ /* assume this is WooF_dir local */
		strncat(local_name, name, sizeof(local_name));
	}
#ifdef DEBUG
	printf("WooFOpenOriginal: trying to open %s from fname %s, %s with dir %s\n",
		   local_name,
		   fname,
		   name,
		   WooF_dir);
	fflush(stdout);
#endif

	if (stat(local_name, &sbuf) < 0)
	{
		fprintf(stderr, "WooFOpenOriginal: couldn't open woof: %s\n", local_name);
		fflush(stderr);
		return (NULL);
	}

	/*
	 * here is the HACK for open woof caching
	 */
	if (WooF_open_cache != NULL)
	{
		wel = WooFCacheFind(WooF_open_cache, local_name);
		if (wel != NULL)
		{
			/*
			 * if the woof hasn't been recreated
			 * it is still "good"
			 */
			if (wel->ino == sbuf.st_ino)
			{
#ifdef DEBUG
				printf("WooFOpenOriginal: found cached woof for %s\n", local_name);
				fflush(stdout);
#endif
				return (wel->wf);
			}
			else
			{
#ifdef DEBUG
				printf("WooFOpenOriginal: expiring cached woof for %s\n", local_name);
				fflush(stdout);
#endif
				WooFCacheRemove(WooF_open_cache, local_name);
				WooFDrop(wel->wf);
				free(wel);
			}
		}
	}

	mio = MIOReOpen(local_name);
	if (mio == NULL)
	{
		return (NULL);
	}
#ifdef DEBUG
	printf("WooFOpenOriginal: opened %s\n", local_name);
	fflush(stdout);
#endif

	wf = (WOOF *)malloc(sizeof(WOOF));
	if (wf == NULL)
	{
		MIOClose(mio);
		return (NULL);
	}
	memset(wf, 0, sizeof(WOOF));

	wf->shared = (WOOF_SHARED *)MIOAddr(mio);
	wf->mio = mio;
	wf->ino = sbuf.st_ino;

	if (WooF_open_cache != NULL)
	{
		wel = (WOOF_OPEN_CACHE_EL *)malloc(sizeof(WOOF_OPEN_CACHE_EL));
		if ((wel != NULL) && (stat(local_name, &sbuf) >= 0))
		{
			wel->wf = wf;
			wel->ino = sbuf.st_ino;
		}
#ifdef DEBUG
		printf("WooFOpenOriginal: inserting cached woof for %s\n", local_name);
		fflush(stdout);
#endif
		err = WooFCacheInsert(WooF_open_cache, local_name, (void *)wel);
		/*
		 * try only once on failure
		 */
		if (err < 0)
		{
			pel = (WOOF_OPEN_CACHE_EL *)WooFCacheAge(WooF_open_cache);
			if (pel != NULL)
			{
				WooFDrop(pel->wf);
				free(pel);
			}
			err = WooFCacheInsert(WooF_open_cache, local_name, (void *)wel);
			if (err < 0)
			{
				free(wel);
			}
		}
	}
	return (wf);
}

/*
 * start to repair a WooF in read-only mode (mapping seq_no for WooFGetLatestSeqNo())
 */
int WooFRepairProgress(char *wf_name, unsigned long cause_host, char *cause_woof, int mapping_count, unsigned long *mapping)
{
	WOOF *wf;
	WOOF_SHARED *wfs;
	unsigned long el_size;
	char wf_namespace[4096];
	char ns_ip[25];
	char my_ip[25];
	char woof_name[4096];
	char *key;
	Hval hval;
	RB *rb;
	unsigned long *seqno_mapping;
	int err;

#ifdef DEBUG
	printf("WooFRepairProgress: called %s\n", wf_name);
	fflush(stdout);
#endif

	key = KeyFromWooF(cause_host, cause_woof);
	seqno_mapping = malloc(2 * mapping_count * sizeof(unsigned long));
	if (seqno_mapping == NULL)
	{
		printf("WooFRepairProgress: couldn't allocate memory for seqno_mapping\n");
		fflush(stdout);
		free(key);
		return (-1);
	}
	memcpy(seqno_mapping, mapping, 2 * mapping_count * sizeof(unsigned long));

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	/*
	 * if there is no IP address in the URI, use the local IP address
	 */
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFRepairProgress: no local IP\n");
			free(key);
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFRepairProgress: no local IP\n");
		free(key);
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote put
	 *
	 * err < 0 implies that name is local name
	 *
	 * if namespace paths do not match or they do match but the IP addresses do not match, this is
	 * a remote put
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		el_size = WooFMsgGetElSize(wf_name);
		if (el_size != (unsigned long)-1)
		{
			err = WooFMsgRepairProgress(wf_name);
			free(key);
			return (err);
		}
		else
		{
			fprintf(stderr, "WooFRepairProgress: couldn't get element size for %s\n",
					wf_name);
			fflush(stderr);
			return (-1);
		}
	}

	memset(woof_name, 0, sizeof(woof_name));
	WooFNameFromURI(wf_name, woof_name, sizeof(woof_name));
	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFRepairProgress: local namespace put must init system\n");
		fflush(stderr);
		free(key);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFRepairProgress: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, woof_name);
	fflush(stdout);
#endif
	wf = WooFOpenOriginal(woof_name);
	if (wf == NULL)
	{
		fprintf(stderr, "WooFRepairProgress: couldn't open woof %s\n", woof_name);
		fflush(stderr);
		free(key);
		return (-1);
	}
	wfs = wf->shared;
	P(&wfs->mutex);

	if ((wfs->repair_mode & SHADOW) || (wfs->repair_mode & REPAIRING))
	{
		fprintf(stderr, "WooFRepairProgress: WooF %s is currently being repaired\n", woof_name);
		fflush(stderr);
		V(&wfs->mutex);
		WooFFree(wf);
		free(key);
		return (-1);
	}

	wfs->repair_mode = READONLY;
	err = WooFShadowMappingCreate(wf_name, cause_host, cause_woof, mapping_count, mapping);
	if (err < 0)
	{
		fprintf(stderr, "WooFRepairProgress: couldn't create shadow mapping %s.%lu_%s\n", woof_name, cause_host, cause_woof);
		fflush(stderr);
		V(&wfs->mutex);
		WooFFree(wf);
		free(key);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFRepairProgress: shadow mapping %s.%lu_%s created\n", woof_name, cause_host, cause_woof);
	fflush(stdout);
#endif

	V(&wfs->mutex);
	WooFFree(wf);
	free(key);
	return (0);
}

int WooFRepair(char *wf_name, Dlist *seq_no)
{
	WOOF *wf;
	WOOF_SHARED *wfs;
	unsigned long el_size;
	char wf_namespace[2048];
	char ns_ip[25];
	char my_ip[25];
	char woof_name[2048];
	char shadow_name[2048];
	int err;

#ifdef DEBUG
	printf("WooFRepair: called %s\n", wf_name);
	fflush(stdout);
#endif

	memset(ns_ip, 0, sizeof(ns_ip));
	err = WooFIPAddrFromURI(wf_name, ns_ip, sizeof(ns_ip));
	/*
	 * if there is no IP address in the URI, use the local IP address
	 */
	if (err < 0)
	{
		err = WooFLocalIP(ns_ip, sizeof(ns_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFRepair: no local IP\n");
			exit(1);
		}
	}

	memset(my_ip, 0, sizeof(my_ip));
	err = WooFLocalIP(my_ip, sizeof(my_ip));
	if (err < 0)
	{
		fprintf(stderr, "WooFRepair: no local IP\n");
		exit(1);
	}

	memset(wf_namespace, 0, sizeof(wf_namespace));
	err = WooFNameSpaceFromURI(wf_name, wf_namespace, sizeof(wf_namespace));
	/*
	 * if this isn't for my namespace, try and remote put
	 *
	 * err < 0 implies that name is local name
	 *
	 * if namespace paths do not match or they do match but the IP addresses do not match, this is
	 * a remote put
	 */
	if ((err >= 0) &&
		((strcmp(WooF_namespace, wf_namespace) != 0) ||
		 (strcmp(my_ip, ns_ip) != 0)))
	{
		el_size = WooFMsgGetElSize(wf_name);
		if (el_size != (unsigned long)-1)
		{
			err = WooFMsgRepair(wf_name, seq_no);
			return (err);
		}
		else
		{
			fprintf(stderr, "WooFRepair: couldn't get element size for %s\n",
					wf_name);
			fflush(stderr);
			return (-1);
		}
	}

	memset(woof_name, 0, sizeof(woof_name));
	WooFNameFromURI(wf_name, woof_name, sizeof(woof_name));
	if (WooF_dir[0] == 0)
	{
		fprintf(stderr, "WooFRepair: local namespace put must init system\n");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFRepair: namespace: %s,  WooF_dir: %s, name: %s\n",
		   WooF_namespace, WooF_dir, woof_name);
	fflush(stdout);
#endif
	wf = WooFOpenOriginal(woof_name);
	if (wf == NULL)
	{
		fprintf(stderr, "WooFRepair: couldn't open woof %s\n", woof_name);
		fflush(stderr);
		return (-1);
	}
	wfs = wf->shared;
	P(&wfs->mutex);

	if ((wfs->repair_mode & SHADOW) || (wfs->repair_mode & REPAIRING) || (wfs->repair_mode & READONLY))
	{
		fprintf(stderr, "WooFRepair: WooF %s is currently being repaired\n", woof_name);
		fflush(stderr);
		V(&wfs->mutex);
		WooFFree(wf);
		return (-1);
	}
	sprintf(shadow_name, "%s_shadow", woof_name);
	err = WooFShadowCreate(shadow_name, woof_name, wfs->element_size, wfs->history_size, seq_no);
	if (err < 0)
	{
		fprintf(stderr, "WooFRepair: cannot create shadow for WooF %s\n", woof_name);
		fflush(stderr);
		V(&wfs->mutex);
		WooFFree(wf);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFRepair: WooF shadow %s created\n", shadow_name);
	fflush(stdout);
#endif

	wfs->repair_mode |= REPAIRING;
	V(&wfs->mutex);
	WooFFree(wf);

	wf = WooFOpenOriginal(shadow_name);
	if (wf == NULL)
	{
		fprintf(stderr, "WooFRepair: couldn't open shadow %s\n", shadow_name);
		fflush(stderr);
		return (-1);
	}
	wfs = wf->shared;
	P(&wfs->mutex); // lock the shadow
	err = WooFShadowForward(wf);
	if (err < 0)
	{
		fprintf(stderr, "WooFRepair: couldn't forward shadow %s\n", shadow_name);
		fflush(stderr);
		V(&wfs->mutex);
		WooFFree(wf);
		return (-1);
	}
	V(&wfs->mutex);
	WooFFree(wf);

	return (0);
}

/*
 * Create a shadow WooF for repair
 */
int WooFShadowCreate(char *name, char *original_name, unsigned long element_size, unsigned long history_size, Dlist *seq_no)
{
	WOOF_SHARED *wfs;
	MIO *mio;
	unsigned long space;
	char local_name[4096];
	char temp_name[4096];
	char fname[1024];
	char my_ip[25];
	char ip_str[25];
	int err;
	int is_local;
	struct stat sbuf;
	struct stat obuf;
	int renamed;
	double r;
	unsigned long *repair_count;
	unsigned long *repair_head;
	unsigned long *repair_seq_no;
	int i;
	DlistNode *dn;

	if (name == NULL || seq_no == NULL)
	{
		return (-1);
	}

	/*
	 * each element gets a seq_no and log index so we can handle
	 * function cancel if we wrap
	 */
	space = ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED) +
			// count and index of seq_no to be repaired and the seq_nos
			(seq_no->count + 2) * sizeof(unsigned long);

	if (WooF_dir == NULL)
	{
		fprintf(stderr, "WooFShadowCreate: must init system\n");
		fflush(stderr);
		exit(1);
	}

	is_local = 0;
	memset(local_name, 0, sizeof(local_name));
	memset(ip_str, 0, sizeof(ip_str));
	/*
	 * if it is a woof:// spec, check to see if the path matches the namespace
	 *
	 * if it does, it is local => use WooF_dir
	 */
	if (WooFValidURI(name))
	{
		err = WooFNameSpaceFromURI(name, local_name, sizeof(local_name));
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowCreate: bad namespace in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		/*
		 * check to see if there is a host spec
		 */
		memset(my_ip, 0, sizeof(my_ip));
		err = WooFLocalIP(my_ip, sizeof(my_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowCreate: no local IP\n");
			fflush(stderr);
			return (-1);
		}
		err = WooFIPAddrFromURI(name, ip_str, sizeof(ip_str));
		if (err < 0)
		{
			strncpy(ip_str, my_ip, sizeof(ip_str));
		}
		if ((strcmp(WooF_namespace, local_name) == 0) &&
			(strcmp(my_ip, ip_str) == 0))
		{
			/*
			 * woof spec for local name space, use WooF_dir
			 */
			is_local = 1;
			memset(local_name, 0, sizeof(local_name));
			strncpy(local_name, WooF_dir, sizeof(local_name));
			if (local_name[strlen(local_name) - 1] != '/')
			{
				strncat(local_name, "/", 1);
			}
			err = WooFNameFromURI(name, fname, sizeof(fname));
			if (err < 0)
			{
				fprintf(stderr, "WooFShadowCreate: bad name in URI %s\n",
						name);
				fflush(stderr);
				return (-1);
			}
			strncat(local_name, fname, sizeof(fname));
		}
	}
	else
	{ /* not URI spec so must be local */
		is_local = 1;
		strncpy(local_name, WooF_dir, sizeof(local_name));
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		strncat(local_name, name, sizeof(local_name));
	}

	if (is_local == 0)
	{
		fprintf(stderr, "WooFShadowCreate: non-local create of %s not supported (yet)\n",
				name);
		fflush(stderr);
		return (-1);
	}

#ifdef NOTRIGHTNOW
	/*
	 * here is where a remote create message would go
	 */
	if (WooFValidURI(name) && (is_local == 0))
	{
		err = WooFNameSpaceFromURI(name, local_name, sizeof(local_name));
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowCreate: bad namespace in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		err = WooFNameFromURI(name, fname, sizeof(fname));
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowCreate: bad name in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		strncat(local_name, fname, sizeof(fname));
	}
	else
	{ /* assume this is WooF_dir local */
		strncpy(local_name, WooF_dir, sizeof(local_name));
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		strncat(local_name, name, sizeof(local_name));
	}
#endif

	/*
	 * here is a HACK for open woof caching.  If the woof being created already exists, rename it so that the new
	 * woof gets a different inode number (guaranteed).  That way, a cached open woof can "tell" (by getting the
	 * inode number from a stat) whether the woof has changed since it was cached
	 */

	renamed = 0;
	if (stat(local_name, &sbuf) >= 0)
	{
		memset(temp_name, 0, sizeof(temp_name));
		r = drand48();
		sprintf(temp_name, "%s.%10.10f", local_name, r);
		rename(local_name, temp_name);
		renamed = 1;
#ifdef DEBUG
		printf("WooFShadowCreate: renamed %s to %s\n", local_name, temp_name);
		fflush(stdout);
#endif
	}

	mio = MIOOpen(local_name, "w+", space);
	if (mio == NULL)
	{
		fprintf(stderr, "WooFShadowCreate: couldn't open %s with space %lu\n", local_name, space);
		fflush(stderr);
		rename(temp_name, local_name);
		return (-1);
	}
#ifdef DEBUG
	if (stat(local_name, &obuf) >= 0)
	{
		printf("WooFShadowCreate: opened %s with inode %lu\n", local_name, obuf.st_ino);
	}
	else
	{
		printf("WooFShadowCreate: opened %s\n", local_name);
	}
	fflush(stdout);
#endif

	if (renamed == 1)
	{
#ifdef DEBUG
		printf("WooFShadowCreate: attempting unlick of %s\n", temp_name);
		fflush(stdout);
#endif
		err = unlink(temp_name);
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowCreate: couldn't dispose of %s\n", temp_name);
			fflush(stderr);
		}
	}

	wfs = (WOOF_SHARED *)MIOAddr(mio);
	memset(wfs, 0, sizeof(WOOF_SHARED));

	wfs->history_size = history_size;
	wfs->element_size = element_size;
	wfs->seq_no = 1;
	wfs->repair_mode = (REPAIRING | SHADOW);
	sprintf(wfs->filename, "%s", original_name);

	InitSem(&wfs->mutex, 1);
	InitSem(&wfs->tail_wait, history_size);

	// put the seq_nos to be repaired to the end of mio
	repair_count = MIOAddr(mio) + ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED);
	repair_head = MIOAddr(mio) + ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED) + sizeof(unsigned long);
	repair_seq_no = MIOAddr(mio) + ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED) + 2 * sizeof(unsigned long);
	*repair_count = seq_no->count;
	*repair_head = 0;
	i = 0;
	DLIST_FORWARD(seq_no, dn)
	{
		repair_seq_no[i++] = dn->value.i64;
	}
	MIOClose(mio);

	return (1);
}

/*
 * Forward the shadow woof, not thread safe
 * lock the original woof mutex when repair is finished
 */
int WooFShadowForward(WOOF *wf)
{
	WOOF *og_wf;
	WOOF_SHARED *wfs;
	MIO *mio;
	unsigned long history_size;
	unsigned long element_size;
	unsigned long latest_seq_no;
	unsigned long *repair_count;
	unsigned long *repair_head;
	unsigned long *repair_seq_no;
	int err;
	unsigned long next;
	unsigned long ndx;
	unsigned long size;
	int i;
	clock_t start;
	int diff;

	wfs = wf->shared;
	history_size = wfs->history_size;
	element_size = wfs->element_size;
	mio = wf->mio;
	repair_count = MIOAddr(mio) + ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED);
	repair_head = MIOAddr(mio) + ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED) + sizeof(unsigned long);
	repair_seq_no = MIOAddr(mio) + ((history_size + 1) * (element_size + sizeof(ELID))) + sizeof(WOOF_SHARED) + 2 * sizeof(unsigned long);

	og_wf = WooFOpenOriginal(wfs->filename); // open the original woof for fast forwarding shadow
	if (og_wf == NULL)
	{
		fprintf(stderr, "WooFShadowForward: couldn't open the original WooF %s\n", wfs->filename);
		fflush(stderr);
		return (-1);
	}

	if (*repair_head < *repair_count)
	{
#ifdef DEBUG
		printf("WooFShadowForward: before: wfs->seq_no: %lu, repair_count: %lu, repair_head: %lu\n", wfs->seq_no, *repair_count, *repair_head);
		fflush(stdout);
#endif
		ndx = WooFIndexFromSeqno(og_wf, wfs->seq_no);
		if (ndx < 0)
		{
			fprintf(stderr, "WooFShadowForward: couldn't convert seq_no %lu to index from WooF %s\n", wfs->seq_no, wfs->filename);
			fflush(stderr);
			WooFFree(og_wf);
			return (-1);
		}
		size = repair_seq_no[*repair_head] - wfs->seq_no;
		if (size > 0)
		{
			err = WooFReplace(wf, og_wf, ndx, size);
			if (err < 0)
			{
				fprintf(stderr, "WooFShadowForward: couldn't copy from shadow woof, ndx: %lu, size: %lu\n", ndx, size);
				fflush(stderr);
				WooFFree(og_wf);
				return (-1);
			}
			next = (wfs->head + size) % wfs->history_size;
			wfs->head = next;
			if (next == wfs->tail)
			{
				wfs->tail = (wfs->tail + 1) % wfs->history_size;
			}
			wfs->seq_no += size;
		}
		*repair_head = (*repair_head) + 1;
#ifdef DEBUG
		printf("WooFShadowForward: after: wfs->seq_no: %lu, repair_count: %lu, repair_head: %lu\n", wfs->seq_no, *repair_count, *repair_head);
		fflush(stdout);
#endif
	}
	else if (wfs->seq_no > repair_seq_no[*repair_count - 1]) // *repair_head == *repair_count
	{
#ifdef DEBUG
		printf("WooFShadowForward: closing shadow, wfs->seq_no: %lu, last_repair_seqno: %Lu\n", wfs->seq_no, repair_seq_no[*repair_count - 1]);
		fflush(stdout);
#endif
		P(&og_wf->shared->mutex);
		latest_seq_no = og_wf->shared->seq_no;
		size = latest_seq_no - wfs->seq_no;
#ifdef DEBUG
		printf("WooFShadowForward: copying from original size: latest_seq_no %lu, wfs->seq_no %lu, size %lu\n",
			   latest_seq_no, wfs->seq_no, size);
		fflush(stdout);
#endif
		if (size > 0)
		{
			ndx = WooFIndexFromSeqno(og_wf, wfs->seq_no);
			if (ndx < 0)
			{
				fprintf(stderr, "WooFShadowForward: couldn't convert seq_no %lu to index from WooF %s\n", wfs->seq_no, wfs->filename);
				fflush(stderr);
				WooFFree(og_wf);
				return (-1);
			}
			err = WooFReplace(wf, og_wf, ndx, size);
			if (err < 0)
			{
				fprintf(stderr, "WooFShadowForward: couldn't copy from shadow woof, ndx: %lu, size: %lu\n", ndx, size);
				fflush(stderr);
				WooFFree(og_wf);
				return (-1);
			}
			next = (wfs->head + size) % wfs->history_size;
			wfs->head = next;
			if (next == wfs->tail)
			{
				wfs->tail = (wfs->tail + 1) % wfs->history_size;
			}
			wfs->seq_no += size;
		}
#ifdef DEBUG
		printf("WooFShadowForward: done copying\n");
		fflush(stdout);
#endif

		// copy back
// #ifdef DEBUG
		printf("WooFShadowForward: copying from shadow to the original woof %s\n", wf->shared->filename);
		fflush(stdout);
		start = clock();
// #endif
		err = WooFReplace(og_wf, wf, 0, wf->shared->history_size);
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowForward: couldn't copy from shadow woof to the original woof: %s\n", og_wf->shared->filename);
			fflush(stderr);
			WooFFree(og_wf);
			return (-1);
		}
// #ifdef DEBUG
		diff = (clock() - start) * 1000 / CLOCKS_PER_SEC;
		printf("WooFShadowForward: done copying from shadow to the original woof %s: %d ms, history_size %lu\n", wf->shared->filename, diff, wf->shared->history_size);
		fflush(stdout);
// #endif
		og_wf->shared->seq_no = wfs->seq_no;
		og_wf->shared->head = wfs->head;
		og_wf->shared->tail = wfs->tail;
		og_wf->shared->repair_mode = NORMAL;

#ifdef DEBUG
		printf("WooFShadowForward: shadow %s closed\n", wfs->filename);
		fflush(stdout);
#endif
		V(&og_wf->shared->mutex);
// #ifdef DEBUG
		printf("WooFShadowForward: marking original events invalid for %s\n", wfs->filename);
		fflush(stdout);
		start = clock();
// #endif
		// mark the original events invalid
		LogInvalidByWooF(Name_log);
// #ifdef DEBUG
		diff = (clock() - start) * 1000 / CLOCKS_PER_SEC;
		printf("WooFShadowForward: done marking invalid: %d ms\n", diff);
		fflush(stdout);
// #endif
		wfs->repair_mode = NORMAL;
	}

	WooFFree(og_wf);
	return (1);
}

/*
 * Create a shadow WooF for repair in read-only mode
 */
int WooFShadowMappingCreate(char *name, unsigned long cause_host, char *cause_woof, int mapping_count, unsigned long *seqno_mapping)
{
	MIO *mio;
	unsigned long space;
	char local_name[4096];
	char fname[1024];
	char my_ip[25];
	char ip_str[25];
	int err;
	int is_local;
	char *key;
	unsigned long *mapping;

	if (name == NULL || cause_woof == NULL || seqno_mapping == NULL)
	{
		return (-1);
	}

	space = (mapping_count * 2 + 1) * sizeof(unsigned long);

	if (WooF_dir == NULL)
	{
		fprintf(stderr, "WooFShadowMappingCreate: must init system\n");
		fflush(stderr);
		exit(1);
	}

	is_local = 0;
	memset(local_name, 0, sizeof(local_name));
	memset(ip_str, 0, sizeof(ip_str));
	/*
	 * if it is a woof:// spec, check to see if the path matches the namespace
	 *
	 * if it does, it is local => use WooF_dir
	 */
	if (WooFValidURI(name))
	{
		err = WooFNameSpaceFromURI(name, local_name, sizeof(local_name));
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowMappingCreate: bad namespace in URI %s\n",
					name);
			fflush(stderr);
			return (-1);
		}
		/*
		 * check to see if there is a host spec
		 */
		memset(my_ip, 0, sizeof(my_ip));
		err = WooFLocalIP(my_ip, sizeof(my_ip));
		if (err < 0)
		{
			fprintf(stderr, "WooFShadowMappingCreate: no local IP\n");
			fflush(stderr);
			return (-1);
		}
		err = WooFIPAddrFromURI(name, ip_str, sizeof(ip_str));
		if (err < 0)
		{
			strncpy(ip_str, my_ip, sizeof(ip_str));
		}
		if ((strcmp(WooF_namespace, local_name) == 0) &&
			(strcmp(my_ip, ip_str) == 0))
		{
			/*
			 * woof spec for local name space, use WooF_dir
			 */
			is_local = 1;
			memset(local_name, 0, sizeof(local_name));
			strncpy(local_name, WooF_dir, sizeof(local_name));
			if (local_name[strlen(local_name) - 1] != '/')
			{
				strncat(local_name, "/", 1);
			}
			err = WooFNameFromURI(name, fname, sizeof(fname));
			if (err < 0)
			{
				fprintf(stderr, "WooFShadowMappingCreate: bad name in URI %s\n",
						name);
				fflush(stderr);
				return (-1);
			}
			strncat(local_name, fname, sizeof(fname));
		}
	}
	else
	{ /* not URI spec so must be local */
		is_local = 1;
		strncpy(local_name, WooF_dir, sizeof(local_name));
		if (local_name[strlen(local_name) - 1] != '/')
		{
			strncat(local_name, "/", 1);
		}
		strncat(local_name, name, sizeof(local_name));
	}

	if (is_local == 0)
	{
		fprintf(stderr, "WooFShadowMappingCreate: non-local create of %s not supported (yet)\n",
				name);
		fflush(stderr);
		return (-1);
	}

	key = KeyFromWooF(cause_host, cause_woof);
	strncat(local_name, ".", sizeof(char));
	strncat(local_name, key, strlen(key));
	mio = MIOOpen(local_name, "w+", space);
	if (mio == NULL)
	{
		fprintf(stderr, "WooFShadowMappingCreate: couldn't open %s with space %lu\n", local_name, space);
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFShadowMappingCreate: readonly shadow mapping %s created\n", local_name);
	fflush(stdout);
#endif

	mapping = (unsigned long *)MIOAddr(mio);
	mapping[0] = mapping_count;
	memcpy(&mapping[1], seqno_mapping, mapping_count * 2 * sizeof(unsigned long));
	MIOClose(mio);
	return (1);
}

/*
 * not thread safe
 */
int WooFReplace(WOOF *dst, WOOF *src, unsigned long ndx, unsigned long size)
{
	void *ptr_dst;
	void *ptr_src;
	unsigned long space;

	if (dst == NULL || src == NULL)
	{
		fprintf(stderr, "WooFReplace: one of the WooF is NULL\n");
		fflush(stderr);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFReplace: called dst: %s src: %s ndx: %lu size %lu\n",
		   dst->shared->filename, src->shared->filename, ndx, size);
	fflush(stdout);
#endif

	if (dst->shared->element_size != src->shared->element_size || dst->shared->history_size != src->shared->history_size)
	{
		fprintf(stderr, "WooFReplace: two WooFs have different size configurations\n");
		fflush(stderr);
		return (-1);
	}
	if (size > dst->shared->history_size)
	{
		fprintf(stderr, "WooFReplace: size %lu is larger than history_size %lu\n", size, dst->shared->history_size);
		fflush(stderr);
		return (-1);
	}
	if (ndx + size > dst->shared->history_size) // if wrapped
	{
		WooFReplace(dst, src, ndx, dst->shared->history_size - ndx);
		WooFReplace(dst, src, 0, size - dst->shared->history_size + ndx);
		return (1);
	}

	ptr_dst = (void *)dst->shared + sizeof(WOOF_SHARED) +
			  ndx * (dst->shared->element_size + sizeof(ELID));
	ptr_src = (void *)src->shared + sizeof(WOOF_SHARED) +
			  ndx * (src->shared->element_size + sizeof(ELID));
	space = size * (src->shared->element_size + sizeof(ELID));
	memcpy(ptr_dst, ptr_src, space);

	return (1);
}

unsigned long WooFIndexFromSeqno(WOOF *wf, unsigned long seq_no)
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
	ptr = buf + (wfs->head * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);
	youngest = el_id->seq_no;

	last_valid = wfs->tail;
	ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);
	oldest = el_id->seq_no;

	if (oldest == 0)
	{ /* haven't wrapped yet */
		last_valid++;
		ptr = buf + (last_valid * (wfs->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr + wfs->element_size);
		oldest = el_id->seq_no;
	}

	if ((seq_no < oldest) || (seq_no > youngest))
	{
		V(&wfs->mutex);
		fprintf(stderr, "WooFIndexFromSeqno: seq_no not in range: seq_no: %lu, oldest: %lu, youngest: %lu\n",
				seq_no, oldest, youngest);
		fflush(stderr);
		return (-1);
	}

	ndx = (last_valid + (seq_no - oldest)) % wfs->history_size;
	return (ndx);
}

void WooFPrintMeta(FILE *fd, char *name)
{
	WOOF *wf;
	WOOF_SHARED *wfs;
	wf = WooFOpen(name);
	if (wf == NULL)
	{
		fprintf(stderr, "could'nt open woof %s\n", name);
		fflush(stderr);
		return;
	}
	wfs = wf->shared;
	fprintf(fd, "wfs->filename: %s\n", wfs->filename);
	fprintf(fd, "wfs->seq_no: %lu\n", wfs->seq_no);
	fprintf(fd, "wfs->history_size: %lu\n", wfs->history_size);
	fprintf(fd, "wfs->head: %lu\n", wfs->head);
	fprintf(fd, "wfs->tail: %lu\n", wfs->tail);
	fprintf(fd, "wfs->element_size: %lu\n", wfs->element_size);
	fflush(fd);
}

void WooFDump(FILE *fd, char *name)
{
	int i;
	WOOF *wf;
	WOOF_SHARED *wfs;
	unsigned char *buf;
	unsigned char *ptr;
	ELID *el_id;
	char str[4096];

	wf = WooFOpenOriginal(name);
	if (wf == NULL)
	{
		fprintf(stderr, "WooFDump: could'nt open woof %s\n", name);
		fflush(stderr);
		return;
	}
	wfs = wf->shared;
	buf = (unsigned char *)(((void *)wfs) + sizeof(WOOF_SHARED));

	fprintf(fd, "WooFDump: tail: %lu, head: %lu\n", wfs->tail, wfs->head);
	for (i = wfs->tail; i != wfs->head; i = (i + 1) % wfs->history_size)
	{
		ptr = buf + (i * (wfs->element_size + sizeof(ELID)));
		el_id = (ELID *)(ptr + wfs->element_size);
		memset(str, 0, sizeof(str));
		strncpy(str, ptr, wfs->element_size);
		fprintf(fd, "[%d]: woof[%d] = %s\n", i, el_id->seq_no, str);
	}
	ptr = buf + (wfs->head * (wfs->element_size + sizeof(ELID)));
	el_id = (ELID *)(ptr + wfs->element_size);
	memset(str, 0, sizeof(str));
	strncpy(str, ptr, wfs->element_size);
	fprintf(fd, "[%d]: woof[%d] = %s\n", wfs->head, el_id->seq_no, str);
	fflush(fd);
}

#endif