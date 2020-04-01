#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "log.h"
#include "woofc.h"
#include "woofc-access.h"
#include "woofc-cache.h"

char WooF_dir[2048];
char WooF_namespace[2048];
char WooF_namelog_dir[2048];
char Host_ip[25];
char Namelog_name[2048];
unsigned long Name_id;
LOG *Name_log;

#define ARGS "M"

WOOF_CACHE *WooF_handler_cache;
struct woof_fork_cache_stc
{
	int hpd[2];
	unsigned long element_size;
	unsigned long history_size;
	unsigned long ino;
};

typedef struct woof_fork_cache_stc WOOF_FORK_EL;
#define WOOF_CONTAINER_MAX_CACHE (100)

static int WooFDone;

#define WOOF_CONTAINER_FORKERS (10)
sema ForkerThrottle;
pthread_mutex_t Tlock;
int Tcount;

void WooFShutdown(int sig)
{
	int val;

	WooFDone = 1;
	while (sem_getvalue(&Name_log->tail_wait, &val) >= 0)
	{
		if (val > 0)
		{
			break;
		}
		V(&Name_log->tail_wait);
	}

	return;
}

void *WooFForker(void *arg);
void *WooFReaper(void *arg);

int WooFContainerInit()
{
	struct timeval tm;
	int err;
	char log_name[2048];
	char log_path[2048];
	char putbuf[25];
	pthread_t tid;
	char *str;
	char *ptr;
	MIO *lmio;
	unsigned long name_id;
	int i;

	gettimeofday(&tm, NULL);
	srand48(tm.tv_sec + tm.tv_usec);

	str = getenv("WOOFC_NAMESPACE");
	if (str == NULL)
	{
		fprintf(stderr, "WooFContainerInit: no namespace specified\n");
		exit(1);
	}
	strncpy(WooF_namespace, str, sizeof(WooF_namespace));
#ifdef DEBUG
	fprintf(stdout, "WooFContainerInit: namespace %s\n", WooF_namespace);
	fflush(stdout);
#endif

	str = getenv("WOOFC_DIR");
	if (str == NULL)
	{
		fprintf(stderr, "WooFContainerInit: couldn't find WOOFC_DIR\n");
		exit(1);
	}

	if (strcmp(str, ".") == 0)
	{
		fprintf(stderr, "WOOFC_DIR cannot be .\n");
		fflush(stderr);
		exit(1);
	}

	if (str[0] != '/')
	{ /* not an absolute path name */
		getcwd(WooF_dir, sizeof(WooF_dir));
		if (str[0] == '.')
		{
			strncat(WooF_dir, &(str[1]),
					sizeof(WooF_dir) - strlen(WooF_dir));
		}
		else
		{
			strncat(WooF_dir, "/", sizeof(WooF_dir) - strlen(WooF_dir));
			strncat(WooF_dir, str,
					sizeof(WooF_dir) - strlen(WooF_dir));
		}
	}
	else
	{
		strncpy(WooF_dir, str, sizeof(WooF_dir));
	}

	if (strcmp(WooF_dir, "/") == 0)
	{
		fprintf(stderr, "WooFContainerInit: WOOFC_DIR can't be %s\n",
				WooF_dir);
		exit(1);
	}

	if (strlen(str) >= (sizeof(WooF_dir) - 1))
	{
		fprintf(stderr, "WooFContainerInit: %s too long for directory name\n",
				str);
		exit(1);
	}

	if (WooF_dir[strlen(WooF_dir) - 1] == '/')
	{
		WooF_dir[strlen(WooF_dir) - 1] = 0;
	}

	memset(putbuf, 0, sizeof(putbuf));
	sprintf(putbuf, "WOOFC_DIR=%s", WooF_dir);
	putenv(putbuf);
#ifdef DEBUG
	fprintf(stdout, "WooFContainerInit: %s\n", putbuf);
	fflush(stdout);
#endif

	str = getenv("WOOF_HOST_IP");
	if (str == NULL)
	{
		fprintf(stderr, "WooFContainerInit: couldn't find local host IP\n");
		exit(1);
	}
	strncpy(Host_ip, str, sizeof(Host_ip));

	str = getenv("WOOF_NAME_ID");
	if (str == NULL)
	{
		fprintf(stderr, "WooFContainerInit: couldn't find name id\n");
		exit(1);
	}
	name_id = strtoul(str, (char **)NULL, 10);

	str = getenv("WOOF_NAMELOG_NAME");
	if (str == NULL)
	{
		fprintf(stderr, "WooFContainerInit: couldn't find namelog name\n");
		exit(1);
	}

	strncpy(Namelog_name, str, sizeof(Namelog_name));

	err = mkdir(WooF_dir, 0600);
	if ((err < 0) && (errno != EEXIST))
	{
		perror("WooFContainerInit");
		exit(1);
	}

	strncpy(WooF_namelog_dir, "/cspot-namelog", sizeof(WooF_namelog_dir));

	memset(log_name, 0, sizeof(log_name));
	sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);

	lmio = MIOReOpen(log_name);
	if (lmio == NULL)
	{
		fprintf(stderr,
				"WooFOntainerInit: couldn't open mio for log %s\n", log_name);
		fflush(stderr);
		exit(1);
	}
	Name_log = (LOG *)MIOAddr(lmio);

	if (Name_log == NULL)
	{
		fprintf(stderr, "WooFContainerInit: couldn't open log as %s, size %d\n", log_name, DEFAULT_WOOF_LOG_SIZE);
		fflush(stderr);
		exit(1);
	}

#ifdef DEBUG
	printf("WooFContainerInit: log %s open\n", log_name);
	fflush(stdout);
#endif

	Name_id = name_id;

	InitSem(&ForkerThrottle, WOOF_CONTAINER_FORKERS);
	pthread_mutex_init(&Tlock, NULL);
	Tcount = WOOF_CONTAINER_FORKERS;

	for (i = 0; i < WOOF_CONTAINER_FORKERS; i++)
	{
		err = pthread_create(&tid, NULL, WooFForker, NULL);
		if (err < 0)
		{
			fprintf(stderr, "couldn't start forker thread\n");
			exit(1);
		}
		pthread_detach(tid);
	}

	err = pthread_create(&tid, NULL, WooFReaper, NULL);
	if (err < 0)
	{
		fprintf(stderr, "couldn't start reaper thread\n");
		exit(1);
	}
	pthread_detach(tid);

	signal(SIGHUP, WooFShutdown);
	return (1);
}

void WooFExit()
{
	WooFDone = 1;
	pthread_exit(NULL);
}

void *WooFReaper(void *arg)
{
	int status;
	int i;
	struct timespec tspec;
	struct timeval then;
	struct timeval now;

	while (1)
	{
		gettimeofday(&now, NULL);
		for (i = 0; i < WOOF_CONTAINER_FORKERS; i++)
		{
			while (waitpid(-1, &status, WNOHANG) > 0)
			{
				/*
				 * Pd in Forker just before the fork
				 */
				then = now;
				gettimeofday(&now, NULL);
				V(&ForkerThrottle);
				pthread_mutex_lock(&Tlock);
				Tcount++;
#ifdef DEBUG
				printf("Reaper: count after increment: %d\n", Tcount);
				fflush(stdout);
#endif
				pthread_mutex_unlock(&Tlock);
			}
		}
		if (then.tv_sec == now.tv_sec)
		{
			tspec.tv_sec = 0;
			tspec.tv_nsec = 5000000;
		}
		else
		{
			tspec.tv_sec = 1;
			tspec.tv_nsec = 0;
		}
		nanosleep(&tspec, NULL);
		then = now;
	}

	pthread_exit(NULL);
}

void *WooFForker(void *arg)
{
	unsigned long last_seq_no = 0;
	unsigned long trigger_seq_no;
	unsigned long ls;
	unsigned long first;
	unsigned long firing;
	unsigned long last;
	LOG *log_tail;
	EVENT *ev;
	EVENT *fev;
	char *launch_string;
	char *pathp;
	WOOF *wf;
	char woof_shepherd_dir[2048];
	int err;
	pthread_t tid;
	int none;
	int firing_found;
	EVENT last_event; /* needed to understand if log tail has changed */
	int status;
	int pid;
	char *pbuf;
	char **eenvp;
	int i;
	char cache_name[4096];
	sig_t old_sig;
	int pd[2];
	WOOF_FORK_EL *ce;
	WOOF_FORK_EL *pe;
	int retries;
	unsigned long cache_vals[2];

	/*
	 * wait for things to show up in the log
	 */
	memset(&last_event, 0, sizeof(last_event));
#ifdef DEBUG
	fprintf(stdout, "WooFForker: namespace: %s started\n",
			WooF_namespace);
	fflush(stdout);
#endif

#ifdef DEBUG
	fprintf(stdout, "WooFForker: namespace: %s memset called\n",
			WooF_namespace);
	fflush(stdout);
#endif

	while (WooFDone == 0)
	{
#ifdef DEBUG
		fprintf(stdout, "WooFForker: namespace: %s caling P\n",
				WooF_namespace);
		fflush(stdout);
#endif
		P(&Name_log->tail_wait);
#ifdef DEBUG
		fprintf(stdout, "WooFForker (%lu): namespace: %s awake\n",
				pthread_self(), WooF_namespace);
		fflush(stdout);
#endif

		//		pthread_yield();

		if (WooFDone == 1)
		{
			break;
		}

		/*
		 * must lock to sync on log tail
		 */
		P(&Name_log->mutex);
#ifdef DEBUG
		fprintf(stdout, "WooFForker (%lu): namespace: %s, in mutex, size: %lu, last: %lu\n",
				pthread_self(), WooF_namespace, Name_log->size, last_seq_no);
		fflush(stdout);
#endif
		log_tail = LogTail(Name_log, last_seq_no, Name_log->size);
		// log_tail = LogTail(Name_log, last_seq_no, Name_log->seq_no - Name_log->last_trigger_seq_no);

		if (log_tail == NULL)
		{
#ifdef DEBUG
			fprintf(stdout, "WooFForker: namespace: %s no tail, continuing\n",
					WooF_namespace);
			fflush(stdout);
#endif
			V(&Name_log->mutex);
			continue;
		}
		if (log_tail->head == log_tail->tail)
		{
#ifdef DEBUG
			fprintf(stdout, "WooFForker (%lu): namespace: %s log tail empty, last: %lu continuing\n",
					pthread_self(), WooF_namespace, last_seq_no);
			fflush(stdout);
#endif
			V(&Name_log->mutex);
			LogFree(log_tail);
			continue;
		}

		ev = (EVENT *)(MIOAddr(log_tail->m_buf) + sizeof(LOG));

		/*
		 * find the first TRIGGER we haven't seen yet
		 * skip triggers for other name spaces but try and wake them up
		 */
		none = 0;
		first = log_tail->head;

		while (first != log_tail->tail)
		{
			/*
			 * is this trigger in my namespace and unclaimed?
			 */
			if ((ev[first].type == TRIGGER) &&
				(strncmp(ev[first].namespace, WooF_namespace, sizeof(ev[first].namespace)) == 0) &&
				(ev[first].seq_no > (unsigned long long)last_seq_no))
			{
				/* now walk forward looking for FIRING */
#ifdef DEBUG
				printf("WooFForker: considering %s %llu\n", ev[first].namespace, ev[first].seq_no);
				fflush(stdout);
#endif
				firing = (first - 1);
				if (firing >= log_tail->size)
				{
					firing = log_tail->size - 1;
				}
				trigger_seq_no = (unsigned long)ev[first].seq_no; /* for FIRING dependency */
				firing_found = 0;
				while (firing != log_tail->tail)
				{
					if ((ev[firing].type == TRIGGER_FIRING) &&
						(strncmp(ev[firing].namespace, WooF_namespace, sizeof(ev[firing].namespace)) == 0) &&
						(ev[firing].cause_seq_no ==
						 (unsigned long long)trigger_seq_no))
					{
						/* found FIRING */
						firing_found = 1;
#ifdef DEBUG
						printf("WooFForker: found firing for %s %llu\n", ev[first].namespace, ev[first].seq_no);
						fflush(stdout);
#endif
						last_seq_no = (unsigned long)ev[first].seq_no;
						break;
					}
					firing = firing - 1;
					if (firing >= log_tail->size)
					{
						firing = log_tail->size - 1;
					}
				}
				if (firing_found == 0)
				{
#ifdef DEBUG
					printf("WooFForker: no firing found for %s %llu\n", ev[first].namespace, ev[first].seq_no);
					fflush(stdout);
#endif
					/* there is a TRIGGER with no FIRING */
					break;
				}
			}

			/*
			 * if this is a trigger belonging to another namespace, try and wake
			 * its container(s)
			 *
			 * Note that this may cause spurious wake ups since this container
			 * can't tell if the others have seen this trigger yet
			 */
			if ((ev[first].type == TRIGGER) &&
				(memcmp(&last_event, &ev[first], sizeof(last_event)) != 0) &&
				(strncmp(ev[first].namespace, WooF_namespace, sizeof(ev[first].namespace)) != 0))
			{
#ifdef DEBUG
				printf("WooFForker: namespace: %s found trigger for evns: %s, name: %s, first: %lu, head: %lu, tail: %lu\n",
					   WooF_namespace, ev[first].namespace, ev[first].woofc_name, first, log_tail->head, log_tail->tail);
				fflush(stdout);
#endif
				exit(1);
				memcpy(&last_event, &ev[first], sizeof(last_event));
				V(&Name_log->tail_wait);
			}
// TODO: only go back to latest triggered
			first = (first - 1);
			if (first >= log_tail->size)
			{
				first = log_tail->size - 1;
			}
			if (first == log_tail->tail)
			{
				none = 1;
				break;
			}
		}

		/*
		 * if no TRIGGERS found
		 */
		if (none == 1)
		{
#ifdef DEBUG
			fprintf(stdout, "WooFForker log tail empty, continuing\n");
			fflush(stdout);
#endif
			V(&Name_log->mutex);
			LogFree(log_tail);
			continue;
		}
		/*
		 * otherwise, fire this event
		 */

#ifdef DEBUG
		fprintf(stdout, "WooFForker (%lu): namespace: %s accepted and firing woof: %s handler: %s woof_seq_no: %lu log_seq_no: %llu\n",
				pthread_self(),
				WooF_namespace,
				ev[first].woofc_name,
				ev[first].woofc_handler,
				ev[first].woofc_seq_no,
				ev[first].seq_no);
		fflush(stdout);
#endif

		// Name_log->last_trigger_seq_no = (unsigned long long)trigger_seq_no;
		/*
		 * before dropping mutex, log a FIRING record
		 */
		fev = EventCreate(TRIGGER_FIRING, Name_id);
		if (fev == NULL)
		{
			fprintf(stderr, "WooFForker: couldn't create TRIGGER_FIRING record\n");
			V(&Name_log->mutex);
			exit(1);
		}
		fev->cause_host = Name_id;
		fev->cause_seq_no = (unsigned long long)trigger_seq_no;
		memset(fev->namespace, 0, sizeof(fev->namespace));
		strncpy(fev->namespace, WooF_namespace, sizeof(fev->namespace));
#ifdef DEBUG
		printf("WooFForker: logging TRIGGER_FIRING for %s %llu\n",
			   ev[first].namespace, ev[first].seq_no);
		fflush(stdout);
#endif
		/*
		 * must be LogAdd() call since inside of critical section
		 */
		ls = LogEventNoLock(Name_log, fev);
		if (ls == 0)
		{
			fprintf(stderr, "WooFForker: couldn't log event to log\n");
			fflush(stderr);
			EventFree(fev);
			V(&Name_log->mutex);
			exit(1);
		}
		EventFree(fev);

		/*
		 * drop the mutex now that this TRIGGER is claimed
		 */
		V(&Name_log->mutex);
#ifdef DEBUG
		fprintf(stdout, "WooFForker: namespace: %s out of mutex with log tail\n",
				WooF_namespace);
		fflush(stdout);
#endif

#ifdef CACHE_ON
		if (WooF_handler_cache == NULL)
		{
			WooF_handler_cache = WooFCacheInit(WOOF_CONTAINER_MAX_CACHE);
			if (WooF_handler_cache == NULL)
			{
				fprintf(stderr, "WooFForker: failed to init handler cache\n");
				fflush(stderr);
				exit(1);
			}
		}
#endif

		memset(cache_name, 0, sizeof(cache_name));
		sprintf(cache_name, "%s.%s", ev[first].woofc_name, ev[first].woofc_handler);
		/*
		 * if we find the pipe descriptor in the cache, try and write it
		 */
		if (WooF_handler_cache != NULL)
		{
			ce = WooFCacheFind(WooF_handler_cache, cache_name);
		}
		else
		{
			ce = NULL;
		}
		if ((ce != NULL) && (ce->element_size == ev[first].woofc_element_size) && (ce->history_size == ev[first].woofc_history_size) &&
			(ce->ino == ev[first].ino))
		{
#ifdef DEBUG
			fprintf(stdout, "WooFForker: found cache entry for %s, el_size: %lu, hsize: %lu\n",
					cache_name, ce->element_size, ce->history_size);
			fflush(stdout);
#endif
			old_sig = signal(SIGPIPE, SIG_IGN);
			/*
			 * do it this way so it goes in one, indivisible write()
			 */
			cache_vals[0] = ev[first].woofc_seq_no;
			cache_vals[1] = ev[first].woofc_ndx;
			err = write(ce->hpd[1], cache_vals, sizeof(cache_vals));
			if (err <= 0)
			{
				if (errno == EPIPE)
				{
#ifdef DEBUG
					fprintf(stdout, "WooFForker: removing cache entry for %s\n", cache_name);
					fflush(stdout);
#endif
					WooFCacheRemove(WooF_handler_cache, cache_name);
					close(ce->hpd[1]);
					free(ce);
					ce = NULL;
				}
				else
				{
					fprintf(stderr, "WooFForker: couldn't write seq_no pd for %s\n",
							cache_name);
					perror("WooFForker: bad pd write");
					WooFCacheRemove(WooF_handler_cache, cache_name);
					close(ce->hpd[1]);
					free(ce);
					ce = NULL;
					signal(SIGPIPE, old_sig);
				}
			}
			else
			{ /* new seq_no sent, continue */
#ifdef DEBUG
				fprintf(stdout, "WooFForker: sending %s new seq_no: %lu and ndx: %lu on fd: %d\n",
						cache_name, ev[first].woofc_seq_no, ev[first].woofc_ndx, ce->hpd[1]);
				fflush(stdout);
#endif
				while (waitpid(-1, &status, WNOHANG) > 0)
					;
				signal(SIGPIPE, old_sig);
				LogFree(log_tail);
				continue;
			}
		}

		/*
		 * if we get here and ce was found, we need to get rid of it
		 */
		if (ce != NULL)
		{
#ifdef DEBUG
			fprintf(stdout, "WooFForker: removing cache entry for %s due to difference, ce: %lu %lu ev: %lu %lu\n",
					cache_name, ce->element_size, ce->history_size,
					ev[first].woofc_element_size, ev[first].woofc_history_size);
			fflush(stdout);
#endif
			WooFCacheRemove(WooF_handler_cache, cache_name);
			close(ce->hpd[1]);
			free(ce);
			ce = NULL;
		}

		/*
		 * here, we need to fork a new process for the handler
		 */

		/*
		 * create a pipe for cache
		 */
		if (WooF_handler_cache != NULL)
		{
			err = pipe(pd);
			ce = NULL;
			if (err >= 0)
			{
				ce = (WOOF_FORK_EL *)malloc(sizeof(WOOF_FORK_EL));
				if (ce == NULL)
				{
					exit(1);
				}
				ce->hpd[0] = pd[0];
				ce->hpd[1] = pd[1];
			}
		}

		/*
		 * block here not to overload the machine
		 */
		pthread_mutex_lock(&Tlock);
#ifdef DEBUG
		printf("Forker calling P with Tcount %d\n", Tcount);
		fflush(stdout);
#endif
		pthread_mutex_unlock(&Tlock);
		P(&ForkerThrottle);
		pthread_mutex_lock(&Tlock);
		Tcount--;
#ifdef DEBUG
		printf("Forker awake, after decrement %d\n", Tcount);
		fflush(stdout);
#endif
		pthread_mutex_unlock(&Tlock);

		pid = fork();
		if (pid == 0)
		{

			/*
		 * I am the child.  I need the read end but not the write end
		 */

			if (ce != NULL)
			{
				dup2(ce->hpd[0], 0);
				close(ce->hpd[0]);
				close(ce->hpd[1]);
				free(ce);
			}
			else
			{
				close(0); /* so shepherd knows there is no pipe */
			}

			wf = WooFOpen(ev[first].woofc_name);

			if (wf == NULL)
			{
				fprintf(stderr, "WooFForker: open failed for WooF at %s, %lu %lu\n",
						ev[first].woofc_name,
						ev[first].woofc_element_size,
						ev[first].woofc_history_size);
				fflush(stderr);
				exit(1);
			}

			/*
		 * find the last directory in the path
		 */
			pathp = strrchr(WooF_dir, '/');
			if (pathp == NULL)
			{
				fprintf(stderr, "couldn't find leaf dir in %s\n",
						WooF_dir);
				exit(1);
			}

			strncpy(woof_shepherd_dir, pathp, sizeof(woof_shepherd_dir));

			eenvp = (char **)malloc(12 * sizeof(char *));
			if (eenvp == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp\n");
				exit(1);
			}

			i = 0; /* 0 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOFC_NAMESPACE=%s", WooF_namespace);
			eenvp[i] = pbuf;
			i++;

			/* 1 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOFC_DIR=%s", WooF_dir);
			eenvp[i] = pbuf;
			i++;

			/* 2 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOF_HOST_IP=%s", Host_ip);
			eenvp[i] = pbuf;
			i++;

			/* 3 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			/*
		 * XXX if we can get the file name in a different way we can eliminate this call to WooFOpen()
		 */
			sprintf(pbuf, "WOOF_SHEPHERD_NAME=%s", wf->shared->filename);
			eenvp[i] = pbuf;
			i++;

			/* 4 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOF_SHEPHERD_NDX=%lu", ev[first].woofc_ndx);
			eenvp[i] = pbuf;
			i++;

			/* 5 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOF_SHEPHERD_SEQNO=%lu", ev[first].woofc_seq_no);
			eenvp[i] = pbuf;
			i++;

			/* 6 */
			pbuf = (char *)malloc(255);
			sprintf(pbuf, "WOOF_NAME_ID=%lu", Name_id);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			eenvp[i] = pbuf;
			i++;

			/* 7 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOF_NAMELOG_DIR=%s", WooF_namelog_dir);
			putenv(pbuf);
			eenvp[i] = pbuf;
			i++;

			/* 8 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOF_NAMELOG_NAME=%s", Namelog_name);
			eenvp[i] = pbuf;
			i++;

			/* 9 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			sprintf(pbuf, "WOOF_NAMELOG_SEQNO=%llu", ev[first].seq_no);
			eenvp[i] = pbuf;
			i++;

			/* 10 */
			pbuf = (char *)malloc(255);
			if (pbuf == NULL)
			{
				fprintf(stderr, "WooFForker: no space for eenvp %d\n", i);
				exit(1);
			}
			memset(pbuf, 0, 255);
			strcpy(pbuf, "LD_LIBRARY_PATH=/usr/local/lib");
			eenvp[i] = pbuf;
			i++;

			/* 11 */
			eenvp[i] = NULL;

			pbuf = (char *)malloc(255);
			sprintf(pbuf, "%s/%s", WooF_dir, ev[first].woofc_handler);

			char *earg[2];
			earg[0] = pbuf;
			earg[1] = NULL;

			WooFFree(wf);

			execve(pbuf, earg, eenvp);

			fprintf(stderr, "WooFForker: execve of %s failed\n", pbuf);
			exit(1);

#if 0
		sprintf(launch_string, "export WOOFC_NAMESPACE=%s; \
			 export WOOFC_DIR=%s; \
			 export WOOF_HOST_IP=%s; \
			 export WOOF_SHEPHERD_NAME=%s; \
			 export WOOF_SHEPHERD_NDX=%lu; \
			 export WOOF_SHEPHERD_SEQNO=%lu; \
			 export WOOF_NAME_ID=%lu; \
			 export WOOF_NAMELOG_DIR=%s; \
			 export WOOF_NAMELOG_NAME=%s; \
			 export WOOF_NAMELOG_SEQNO=%lu; \
			 %s/%s",
				WooF_namespace,
				WooF_dir,
				Host_ip,
				wf->shared->filename,
				ev[first].woofc_ndx,
				ev[first].woofc_seq_no,
				Name_id,
				WooF_namelog_dir,
				Namelog_name,
				ev[first].seq_no,
				WooF_dir,ev[first].woofc_handler);
#endif
		}
		else if (pid < 0)
		{
			fprintf(stderr, "WooFForker: fork failed for %s/%s in %s/%s\n",
					WooF_dir, ev[first].woofc_handler, WooF_namespace, wf->shared->filename);
			fflush(stderr);
			WooFDone = 1;
		}
		else
		{ /* parent */

			if (ce != NULL)
			{
				/* don't need the read end */
				ce->element_size = ev[first].woofc_element_size;
				ce->history_size = ev[first].woofc_history_size;
				ce->ino = ev[first].ino;
				err = WooFCacheInsert(WooF_handler_cache, cache_name, (void *)ce);
				retries = 0;
				if (err < 0)
				{
					while (retries < 10)
					{
						pe = (WOOF_FORK_EL *)WooFCacheAge(WooF_handler_cache);
						if (pe != NULL)
						{
							close(pe->hpd[1]);
							free(pe);
						}
						err = WooFCacheInsert(WooF_handler_cache, cache_name, (void *)ce);
						if (err >= 0)
						{
							break;
						}
						retries++;
					}
				}
				if (retries >= 10)
				{
					free(ce);
					ce = NULL;
				}
				else
				{
					close(ce->hpd[0]);
				}
			}

			/*
			 * remember its sequence number for next time
			 */
			last_seq_no = (unsigned long)ev[first].seq_no; /* log seq_no */
#ifdef DEBUG
			fprintf(stdout, "WooFForker: namespace: %s seq_no: %llu, handler: %s\n",
					WooF_namespace, ev[first].seq_no, ev[first].woofc_handler);
			fflush(stdout);
#endif
			LogFree(log_tail);
		}

		while (waitpid(-1, &status, WNOHANG) > 0)
		{
			V(&ForkerThrottle);
			pthread_mutex_lock(&Tlock);
			Tcount++;
#ifdef DEBUG
			printf("Parent: count after increment: %d\n", Tcount);
			fflush(stdout);
#endif
			pthread_mutex_unlock(&Tlock);
		}
	}

	fprintf(stderr, "WooFForker namespace: %s exiting\n", WooF_namespace);
	fflush(stderr);

	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int err;
	char c;
	int message_server = 0;

	WooFContainerInit();

	/*
	 * check c != 255 for Raspberry Pi
	 */
	while ((c = getopt(argc, argv, ARGS)) != EOF && c != 255)
	{
		switch (c)
		{
		case 'M':
			message_server = 1;
			break;
		}
	}

#ifdef DEBUG
	printf("woofc-container: about to start message server with namespace %s\n",
		   WooF_namespace);
	fflush(stdout);
#endif

	/*
	 * start the msg server for this container
	 * 
	 * for now, this doesn't ever return 
	 */
	if (message_server)
	{
		fprintf(stdout, "woofc-container: started message server\n");
		err = WooFMsgServer(WooF_namespace);
	}
	else
	{
		fprintf(stdout, "woofc-container: message server disabled. not listening.\n");
	}
	fflush(stdout);

	pthread_exit(NULL);
}
