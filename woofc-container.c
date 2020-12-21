#define _GNU_SOURCE
#include <stdlib.h>
#include <fcntl.h>
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

static int WooFDone;


#define WOOF_CONTAINER_FORKERS (16)
sema ForkerThrottle;
pthread_mutex_t Tlock;
int Tcount;

struct forker_stc
{
	int tid;
	int parenttochild[2];
	int childtoparent[2];
};

typedef struct forker_stc FARG;

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

int WooFContainerInit()
{
	struct timeval tm;
	int err;
	char log_name[2048];
	char log_path[2048];
	char putbuf[25];
	pthread_t tid;
	char *str;
	MIO *lmio;
	unsigned long name_id;
	int i;
	FARG *tas;
	char *cargv[2];
	char hbuff[255];
	int pid;

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

	/*
	 * create pipes and spawn forker helper processes
	 */
	tas = (FARG *)malloc(WOOF_CONTAINER_FORKERS*sizeof(FARG));
	if(tas == NULL) {
		exit(1);
	}

	for(i=0; i < WOOF_CONTAINER_FORKERS; i++) {
		tas[i].tid = i;
		err = pipe2(tas[i].parenttochild,O_DIRECT);
		if(err < 0) {
			exit(1);
		}
		err = pipe2(tas[i].childtoparent,O_DIRECT);
		if(err < 0) {
			exit(1);
		}

		sprintf(hbuff,"%s/%s",WooF_dir,"woofc-forker-helper");
		cargv[0] = hbuff;
		cargv[1] = NULL;
		pid = fork();
		if(pid < 0) {
			fprintf(stderr,"WooFContainer: fork failed at %d\n",i);
			perror("WooFContainer");
			exit(0);
		}
		if(pid == 0) {
			/*
			 * child code here
			 */
#ifdef DEBUG
			fprintf(stdout,"WooFContainer: child %d with pid %d running\n",i,getpid());
			fflush(stdout);
#endif
			
			close(tas[i].parenttochild[1]); // this end, the parent writes
			close(tas[i].childtoparent[0]); // this end, the parent reads
			close(0); // child will read stdin
			dup2(tas[i].parenttochild[0],0); // child read end in stdin
			close(2); // child will write back to parent on stderr
			dup2(tas[i].childtoparent[1],2); // write end for child in stderr
#ifdef DEBUG
			fprintf(stdout,"WooFContainer: child about to exec %s\n",hbuff);
			fflush(stdout);
#endif
//			execve(hbuff,cargv,NULL);
			system(hbuff);
			fprintf(stdout,"WooFContainer: execve of %s failed\n",hbuff);
			fflush(stdout);
			close(2);
			dup2(1,2);
			perror("WooFContainer");
			exit(1);
		}
#ifdef DEBUG
		fprintf(stdout,"WooFContainer: parent %d continuing\n",i);
		fflush(stdout);
#endif

		/*
		 * parent code starts here
		 */
		close(tas[i].childtoparent[1]); // child writes this end
		close(tas[i].parenttochild[0]); // child reads this end
	}

	/*
	 * now create the forker threads
	 */

	for (i = 0; i < WOOF_CONTAINER_FORKERS; i++)
	{
		err = pthread_create(&tid, NULL, WooFForker, (void *)&(tas[i]));
		if (err < 0)
		{
			fprintf(stderr, "couldn't start forker thread\n");
			exit(1);
		}
		pthread_detach(tid);
	}


	signal(SIGHUP, WooFShutdown);
	return (1);
}

void WooFExit()
{
	WooFDone = 1;
	pthread_exit(NULL);
}

void *WooFForker(void *arg)
{
	FARG *ta = (FARG *)arg;
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
	sig_t old_sig;
	int pd[2];
	int retries;
	unsigned long cache_vals[2];
	int parenttochild[2];
	int childtoparent[2];
	char *fargv[2];
	char hbuff[255];
	char c;

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

		/*
		 * send the environment variables to the helper
		 */

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
#ifdef DEBUG
		fprintf(stdout,"WooFForker: sending env to helper\n");
		fflush(stdout);
#endif

		/* 0 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOFC_NAMESPACE=%s", WooF_namespace);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write [%d] %s\n",ta->parenttochild[1],hbuff);
			perror("WoofForker");
			exit(1);
		}

		/* 1 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOFC_DIR=%s", WooF_dir);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 2 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_HOST_IP=%s", Host_ip);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 3 */
	/*
	 * XXX if we can get the file name in a different way we can eliminate this call to WooFOpen()
	 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_SHEPHERD_NAME=%s", wf->shared->filename);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 4 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_SHEPHERD_NDX=%lu", ev[first].woofc_ndx);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 5 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_SHEPHERD_SEQNO=%lu", ev[first].woofc_seq_no);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 6 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_NAME_ID=%lu", Name_id);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 7 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_NAMELOG_DIR=%s", WooF_namelog_dir);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 8 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_NAMELOG_NAME=%s", Namelog_name);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 9 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "WOOF_NAMELOG_SEQNO=%llu", ev[first].seq_no);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/* 10 */
		memset(hbuff,0,sizeof(hbuff));
		strcpy(hbuff, "LD_LIBRARY_PATH=/usr/local/lib");
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}

		/*
		 * now send handler
		 */
		memset(hbuff,0,sizeof(hbuff));
		sprintf(hbuff, "%s/%s", WooF_dir, ev[first].woofc_handler);
		err = write(ta->parenttochild[1],hbuff,strlen(hbuff)+1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: failed to write %s\n",hbuff);
			exit(1);
		}


		WooFDrop(wf);

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
		printf("%s\n",launch_string);
		fflush(stdout);
#endif


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

#ifdef DEBUG
		fprintf(stdout,"WooFForker: about to wait for helper signal\n");
		fflush(stdout);
#endif
		/*
		 * wait for child to signal successful exec
		 */
		err = read(ta->childtoparent[0],&c,1);
		if(err <= 0) {
			fprintf(stderr,"WooFForker: woofc-forker-helper closed\n");
			exit(1);
		}
#ifdef DEBUG
		fprintf(stdout,"WooFForker: helper signal received");
		fflush(stdout);
#endif
		V(&ForkerThrottle);
		pthread_mutex_lock(&Tlock);
		Tcount++;
#ifdef DEBUG
		printf("WooFForker: count after increment: %d\n", Tcount);
		fflush(stdout);
#endif
		pthread_mutex_unlock(&Tlock);

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

	signal(SIGPIPE, SIG_IGN);

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
