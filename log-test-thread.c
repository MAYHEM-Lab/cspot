#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "log.h"

#define ARGS "c:f:p:s:S:t:"
char *Usage = "log-test-thread -f filename\n\
\t-c count (number of events on each host)\n\
\t-p probability of a remote event\n\
\t-s size (in events)\n\
\t-S sync count (number of events before updating log)\n\
\t-t threads (one per host)\n";

#define NAMELEN (1024)

char Fname[NAMELEN];

double Prob;
int Sync_count;
int Threads;
int Count;

/*
 * for simulated network
 */
RB *Network;
sema NetworkSema;

/*
 * network is global
 */
int SendEvent(unsigned long recv_host, EVENT *ev)
{
	EVENT *l_ev;

	P(&NetworkSema);
	l_ev = EventCreate(ev->type,ev->host);
	if(l_ev == NULL) {
		fprintf(stderr,"SendEvent couldn't make event\n");
		V(&NetworkSema);
		exit(1);
	}
	l_ev->cause_host = ev->cause_host;
	l_ev->cause_seq_no = ev->cause_seq_no;
	RBInsertD(Network,(double)recv_host,(Hval)(void *)l_ev);
	V(&NetworkSema);

	return(1);
}

EVENT *RecvEvent(unsigned long me)
{
	RB *rb;
	EVENT *ev;

	P(&NetworkSema);

	rb = RBFindD(Network,(double)me);
	if(rb == NULL) {
		V(&NetworkSema);
		return(NULL);
	}

	ev = (EVENT *)rb->value.v;
	RBDeleteD(Network,rb);
	V(&NetworkSema);

	return(ev);

}

/*
 * make a global log from all local logs
 */

int SyncLogs(GLOG **glogs, LOG **llogs, unsigned long host_id, unsigned long max_host)
{
	GLOG *gl;
	LOG *ll;
	int i;
	int err;

	gl = glogs[host_id-1];
	if(gl == NULL) {
		return(-1);
	}

	for(i=0; i < max_host-1; i++) {
		ll = llogs[i];
		if(ll == NULL) {
			return(-1);
		}
		err = ImportLogTail(gl,ll);
		if(err < 0) {
		fprintf(stderr, "thread %lu failed to sync log for host %d\n",
			host_id,
			i+1);
			fflush(stderr);
			return(-1);
		}
	}

	return(1);
}

struct worker_stc
{
	int host_id;
	int max_host_id;
	LOG **llogs;
	GLOG **glogs;
	double prob;
	int sync;
	int count;
};

typedef struct worker_stc WARG;

void *WorkerThread(void *arg)
{
	WARG *warg = (WARG *)arg;
	int i;
	EVENT *ev;
	int until_sync;
	int is_remote;
	unsigned long long seq_no;
	unsigned long long last_seq_no;
	int remote_host;
	int err;

	until_sync = warg->sync;
	last_seq_no = 0;
	for(i=0; i < warg->count; i++) {
		/*
		 * is this a remote event?
		 */
		if(drand48() < warg->prob) {
			is_remote = 1;
		} else {
			is_remote = 0;
		}

		if(is_remote == 0) {
			/*
			 * fire an event locally
			 */
			ev = EventCreate(FUNC,warg->host_id);
			if(ev == NULL) {
				fprintf(stderr,
				"thread: %d couldn't create local event %d\n",
					warg->host_id,i);
				fflush(stderr);
				exit(1);
			}
			ev->cause_host = warg->host_id;
			ev->cause_seq_no = last_seq_no;
			seq_no = LogEvent(warg->llogs[warg->host_id-1],ev);
			if(seq_no == 0) {
				fprintf(stderr,
				"thread: %d couldn't log local event %d\n",
					warg->host_id,i);
				fflush(stderr);
				exit(1);
			}
			last_seq_no = seq_no;
			EventFree(ev);
		} else { /* this is a remote event */
			/*
			 * choose a remote host at random
			 */
			remote_host = (int)(drand48()*(warg->max_host_id-1))+1;
			/*
			 * create and log TRIGGER event locally
			 */
			ev = EventCreate(TRIGGER,warg->host_id);
			if(ev == NULL) {
				fprintf(stderr,
				"thread: %d couldn't create local trigger %d\n",
					warg->host_id,i);
				fflush(stderr);
				exit(1);
			}
			ev->cause_host = warg->host_id;
			ev->cause_seq_no = last_seq_no;
			seq_no = LogEvent(warg->llogs[warg->host_id-1],ev);
			if(seq_no == 0) {
				fprintf(stderr,
				"thread: %d couldn't log local event %d\n",
					warg->host_id,i);
				fflush(stderr);
				exit(1);
			}
			last_seq_no = seq_no;
			EventFree(ev);
			/*
			 * now fire event on remote host
			 */
			ev = EventCreate(FUNC,remote_host);
			if(ev == NULL) {
				fprintf(stderr,
				"thread: %d couldn't create fire event %d on host %d\n",
					warg->host_id,i,remote_host);
				fflush(stderr);
				exit(1);
			}
			ev->cause_host = warg->host_id;
			ev->cause_seq_no = last_seq_no;
			err = SendEvent(remote_host,ev);
			if(err < 0) {
				fprintf(stderr,
				"thread: %d couldn't create fire event %d on host %d\n",
					warg->host_id,i,remote_host);
				fflush(stderr);
				exit(1);
			}
			EventFree(ev);
		}

		/*
		 * now see if there is an inbound event
		 */
		ev = RecvEvent(warg->host_id);
		if(ev != NULL) {
			seq_no = LogEvent(warg->llogs[warg->host_id-1],ev);
			if(seq_no == 0) {
				fprintf(stderr,
				"thread: %d couldn't log remote event %d from %lu\n",
					warg->host_id,i,
					ev->cause_host);
				fflush(stderr);
				exit(1);
			}
			last_seq_no = seq_no;
			EventFree(ev);
		}
		/*
		 * now see if it is sync time
		 */
		if(until_sync == 0) {
			err = SyncLogs(warg->glogs,
				       warg->llogs,
				       warg->host_id,
				       warg->max_host_id);
			if(err < 0) {
				fprintf(stderr,
		"thread %d couldn't sync logs at event %d\n",
				warg->host_id,i);
				exit(1);
			}
			until_sync = warg->sync;
		} else {
			until_sync--;
		}
	}

	/*
	 * mop up any inbound and sync
	 */
	while((ev = RecvEvent(warg->host_id)) != NULL) {
		seq_no = LogEvent(warg->llogs[warg->host_id-1],ev);
		if(seq_no == 0) {
			fprintf(stderr,
			"thread: %d couldn't mop log remote event %d from %lu\n",
				warg->host_id,i,
				ev->cause_host);
			fflush(stderr);
			exit(1);
		}
		last_seq_no = seq_no;
		EventFree(ev);
	}
	err = SyncLogs(warg->glogs, warg->llogs, warg->host_id, warg->max_host_id);
	if(err < 0) {
		fprintf(stderr,
			"thread %d couldn't sync logs at event %d\n",
			warg->host_id,i);
		exit(1);
	}
	free(warg);

	pthread_exit(NULL);
	return(NULL);
}


void PrintGLogDiffs(GLOG **glogs, int log_count)
{
	int i;
	int j;
	unsigned long curr;
	LOG *l1;
	LOG *l2;

	for(i=0; i < log_count; i++) {
		l1 = glogs[i]->log;
		for(j=i+1; j < log_count; j++) {
			l2 = glogs[j]->log;
			if(l1->head != l2->head) {
				printf("%d head %lu %d head %lu\n",
					i+1,
					l1->head,
					j+1,
					l2->head);
				fflush(stdout);
				continue;
			}
			curr = l1->head;
			while(curr != l1->tail) {
				if(curr == l2->tail) {
	printf("%d at %lu but %d has tail %lu\n",
					i+1,
					curr,
					j+1,
					l2->tail);
					fflush(stdout);
					break;
				}
				if(!LogEventEqual(l1,l2,curr)) {
	printf("%d and %d differ at index %lu\n",
					i+1,
					j+1,
					curr);
					fflush(stdout);
					break;
				}
				curr = curr - 1;
				if(curr >= l1->size) {
					curr = l1->size - 1;
				}
			}
		}
	}

	return;
}

	
	
	

int main(int argc, char **argv)
{
	int c;
	char **lnames;
	char **gnames;
	LOG **llogs;
	GLOG **glogs;
	int size;
	int err;
	int i;
	char extent[NAMELEN];
	pthread_t *tids;
	WARG *warg;

	size = 1000;
	Threads = 1;
	Sync_count = 10;
	Prob = 0.1;
	Count = 1;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'c':
				Count = atoi(optarg);
				break;
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'p':
				Prob = atof(optarg);
				break;
			case 'S':
				Sync_count = atoi(optarg);
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 't':
				Threads = atoi(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify file name\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(Count <= 0) {
		fprintf(stderr,"must specify positive cpount of events\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	/*
	 * init the network
	 */
	Network = RBInitD();
	if(Network == NULL) {
		exit(1);
	}
	(void)InitSem(&NetworkSema,1);

	/*
	 * make one log per thread
	 */
	lnames = (char **)malloc(Threads*sizeof(char *));
	if(lnames == NULL) {
		exit(1);
	}
	gnames = (char **)malloc(Threads*sizeof(char *));
	if(gnames == NULL) {
		exit(1);
	}

	llogs = (LOG **)malloc(Threads*sizeof(LOG *));
	if(llogs == NULL) {
		exit(1);
	}
	glogs = (GLOG **)malloc(Threads*sizeof(GLOG *));
	if(glogs == NULL) {
		exit(1);
	}

	tids = (pthread_t *)malloc(Threads*sizeof(pthread_t));
	if(tids == NULL) {
		exit(1);
	}

	for(i=0; i < Threads; i++) {
		lnames[i] = (char *)malloc(NAMELEN);
		if(lnames[i] == NULL) {
			exit(1);
		}
		memset(lnames[i],0,NAMELEN);
		gnames[i] = (char *)malloc(NAMELEN);
		if(gnames[i] == NULL) {
			exit(1);
		}
		memset(gnames[i],0,NAMELEN);

		memset(extent,0,sizeof(extent));
		sprintf(extent,"%s.%d.localmio",
			Fname,i+1);
		strcpy(lnames[i],extent);
		
		memset(extent,0,sizeof(extent));
		sprintf(extent,"%s.%d.globalmio",
			Fname,i+1);
		strcpy(gnames[i],extent);

		llogs[i] = LogCreate(lnames[i],i+1,size);
		if(llogs[i] == NULL) {
			fprintf(stderr,"couldn't create log %d\n",i);
			fflush(stderr);
			exit(1);
		}
		glogs[i] = GLogCreate(gnames[i],size);
		if(glogs[i] == NULL) {
			fprintf(stderr,"couldn't create glog %d\n",i);
			fflush(stderr);
			exit(1);
		}
	}

	for(i=0; i < Threads; i++) {
		warg = (WARG *)malloc(sizeof(WARG));
		if(warg == NULL) {
			exit(1);
		}
		warg->host_id = i+1;
		warg->llogs = llogs;
		warg->glogs = glogs;
		warg->prob = Prob;
		warg->count = Count;
		warg->max_host_id = Threads+1;
		warg->sync = Sync_count;
		err = pthread_create(&tids[i],NULL,WorkerThread,(void *)warg);
		if(err < 0) {
			perror("couldn't create thread\n");
			exit(1);
		}
	}

	for(i=0; i < Threads; i++) {
		err = pthread_join(tids[i],NULL);
		if(err < 0) {
			perror("couldn't join with thread\n");
			exit(1);
		}
	}

	/*
	 * now that the threads are all done, sync one last time
	 */
	for(i=0; i < Threads; i++) {
		err = SyncLogs(glogs,llogs,
			i+1,
			Threads+1);
		if(err < 0) {
			fprintf(stderr,
		"final log sync failed\n");
			exit(1);
		}
	}

	for(i=0; i < Threads; i++) {
		GLogPrint(stdout,glogs[i]);
		fflush(stdout);
	}

	PrintGLogDiffs(glogs,Threads);
	
	for(i=0; i < Threads; i++) {
		if(llogs[i] != NULL) {
			LogFree(llogs[i]);
		}
		if(glogs[i] != NULL) {
			GLogFree(glogs[i]);
		}
		if(lnames[i] != NULL) {
			free(lnames[i]);
		}
		if(gnames[i] != NULL) {
			free(gnames[i]);
		}
	}

	free(llogs);
	free(glogs);
	free(lnames);
	free(gnames);

	return(0);
}
