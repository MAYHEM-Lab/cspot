#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "log.h"

#define ARGS "f:s:"
char *Usage = "log-test -f filename\n\
\t-s size (in events)\n";

char Fname[4096];
char Gname[4096];

char RFname[4096];
char RGname[4096];

/*
 * add events to GLOG from log tail extracted from some local log
 */
int ImportLogTail(GLOG *gl, LOG *ll, unsigned long remote_host)
{
	LOG *lt;
	unsigned long earliest;
	HOST *host;
	unsigned long curr;
	EVENT *ev;
	int err;

	/*
	 * find the max seq_no from the remote gost that the GLOG has seen
	 */
	host = HostListFind(gl->host_list,remote_host);
	if(host == NULL) {
		fprintf(stderr,"couldn't find host %lu\n",remote_host);
		fflush(stderr);
		return(0);
	}

	earliest = host->max_seen;

	lt = LogTail(ll,earliest,100);
	if(lt == NULL) {
		fprintf(stderr,"couldn't get log tail from local log\n");
		fflush(stderr);
		return(0);
	}

	/*
	 * do the import
	 */
	curr = lt->head;
	ev = (EVENT *)(MIOAddr(lt->m_buf) + sizeof(LOG));
	while(ev[curr].seq_no > earliest) {
		err = GLogEvent(gl,&ev[curr]);
		if(err < 0) {
			fprintf(stderr,"couldn't add seq_no %llu\n",
				ev[curr].seq_no);
			fflush(stderr);
			LogFree(lt);
			return(0);
		}
		if(curr == lt->tail) {
			break;
		}
		curr = curr - 1;
		if(curr >= lt->size) {
			curr = (lt->size-1);
		}
	}

	LogFree(lt);

	return(1);
}
	

int main(int argc, char **argv)
{
	int c;
	int size;
	LOG *llog;
	LOG *llog_r;
	GLOG *glog;
	GLOG *glog_r;
	EVENT *ev;
	EVENT *t_ev;
	EVENT *f_ev;
	unsigned long long seq_no;
	int err;
	int i;
	unsigned long long trigger_seq_no;
	unsigned long long trigger_cause_seq_no;
	unsigned long long last_host_1;

	size = 20;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 's':
				size = atoi(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for backing store\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	strcpy(Gname,"global.");
	strcat(Gname,Fname);

	llog = LogCreate(Fname,size);
	if(llog == NULL) {
		fprintf(stderr,"couldn't create local log %s\n",
			Fname);
		fflush(stderr);
		exit(1);
	}

	glog = GLogCreate(Gname,size);
	if(glog == NULL) {
		fprintf(stderr,"couldn't create global log %s\n",
			Gname);
		fflush(stderr);
		exit(1);
	}



	ev = EventCreate(FUNC,1);
	seq_no = LogEvent(llog,ev);
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"first glogevent failed\n");
		exit(1);
	}
	EventFree(ev);

	/*
	 * log some local events
	 */
	for(i=0; i < size; i++) {
		ev = EventCreate(FUNC,1);
		ev->cause_host = 1;
		ev->cause_seq_no = seq_no;
		seq_no = LogEvent(llog,ev);
		err = GLogEvent(glog,ev);
		if(err < 0) {
			fprintf(stderr,
			"glogevent %d failed\n",i);
			exit(1);
		}
		LogPrint(stdout,glog->log);
		EventFree(ev);
	}

	/*
	 * now simulate a remote event == trigger local, event remote
	 */
	strcpy(RFname,"remote.");
	strcat(RFname,Fname);
	strcpy(RGname,"remoteglobal.");
	strcat(RGname,Fname);

	llog_r = LogCreate(RFname,size);
	if(llog_r == NULL) {
		fprintf(stderr,"couldn't create remote local log\n");
		exit(1);
	}

	glog_r = GLogCreate(RGname,size);
	if(glog_r == NULL) {
		fprintf(stderr,"couldn't create remote glog\n");
		exit(1);
	}

	ev = EventCreate(TRIGGER,1);
	ev->cause_host = 1;
	ev->cause_seq_no = seq_no;
	trigger_cause_seq_no = seq_no;
	seq_no = LogEvent(llog,ev);
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"couldn't log trigger on host 1\n");
		exit(1);
	}
	EventFree(ev);

	trigger_seq_no = seq_no;

	/*
	 * now trigger remote event on host 2
	 */
	ev = EventCreate(FUNC,2);
	ev->cause_host = 1;
	ev->cause_seq_no = seq_no;
	seq_no = LogEvent(llog_r,ev);
	err = GLogEvent(glog_r,ev);	/* resolve immediately */
					/* first time */
	if(err < 0) {
		fprintf(stderr,"first dependency event failed\n");
		exit(1);
	}
	EventFree(ev);
	GLogPrint(stdout,glog_r);

	/*
	 * now, later, TRIGGER event arrives from host 1
	 */
	ev = EventCreate(TRIGGER,1);
	ev->seq_no = trigger_seq_no;
	ev->cause_seq_no = trigger_cause_seq_no;
	ev->cause_host = 1;
	err = GLogEvent(glog_r,ev);		/* should do nothing */
	if(err < 0) {
		fprintf(stderr,"clear dependency failed on host 2\n");
		exit(1);
	}
	EventFree(ev);
	GLogPrint(stdout,glog_r);

	/*
	 * now create real dependency
	 */
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = trigger_seq_no;
	seq_no = LogEvent(llog,ev);
	f_ev = ev;
	last_host_1 = seq_no;
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"couldn't create second dep on host 1\n");
		exit(1);
	}
	ev = EventCreate(TRIGGER,1);
	ev->cause_host = 1;
	ev->cause_seq_no = seq_no;
	seq_no = LogEvent(llog,ev);
	t_ev = ev;
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"couldn't log second trigger on host 1\n");
		exit(1);
	}

	/*
	 * fire on host 2
	 */
	ev = EventCreate(FUNC,2);
	ev->cause_host = 1;
	ev->cause_seq_no = seq_no;
	seq_no = LogEvent(llog_r,ev);
	err = GLogEvent(glog_r,ev);
	if(err < 0) {
		fprintf(stderr,"error logging triggerd function on host 2\n");
		exit(1);
	}
	EventFree(ev);

	/*
	 * later TRIGGER arrives from host 1 -- should create dependency
	 */
	err = GLogEvent(glog_r,t_ev);
	GLogPrint(stdout,glog_r);
	if(err < 0) {
		fprintf(stderr,"error logging later trigger arrival\n");
		exit(1);
	}
		

	/*
	 * and then the dependency for the trigger arrives.  This should 
	 * clear the dependencies on host 2
	 */
	err = GLogEvent(glog_r,f_ev);
	if(err < 0) {
		fprintf(stderr,
			"error logging second function arrival at host 2\n");
		exit(1);
	}
	GLogPrint(stdout,glog_r);

	EventFree(t_ev);
	EventFree(f_ev);

	/*
	 * log a bunch of unrelated events on host 1
	 */
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"could log first unrelated event on host 1\n");
		fflush(stderr);
		exit(1);
	}
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"could log second unrelated event on host 1\n");
		fflush(stderr);
	}
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"could log third unrelated event on host 1\n");
		fflush(stderr);
		exit(1);
	}

	/*
	 * now try to import host 1 log on host 2
	 */
	err = ImportLogTail(glog_r,llog,1);
	if(err < 0) {
		fprintf(stderr,"first host 2 import failed\n");
		exit(1);
	}
	GLogPrint(stdout,glog_r);


	LogFree(llog_r);
	LogFree(llog);
	GLogFree(glog_r);
	GLogFree(glog);

	return(0);
}
