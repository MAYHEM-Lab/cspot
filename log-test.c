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
	unsigned long long trigger_reason_seq_no;

	size = 5;
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

	glog = GLogCreate(Gname,size*3);
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
	for(i=0; i < size + 2; i++) {
		ev = EventCreate(FUNC,1);
		ev->reason_host = 1;
		ev->reason_seq_no = seq_no;
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

	glog_r = GLogCreate(RGname,size*3);
	if(glog_r == NULL) {
		fprintf(stderr,"couldn't create remote glog\n");
		exit(1);
	}

	ev = EventCreate(TRIGGER,1);
	ev->reason_host = 1;
	ev->reason_seq_no = seq_no;
	trigger_reason_seq_no = seq_no;
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
	ev->reason_host = 1;
	ev->reason_seq_no = seq_no;
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
	ev->reason_seq_no = trigger_reason_seq_no;
	ev->reason_host = 1;
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
	ev->reason_host = 1;
	ev->reason_seq_no = trigger_seq_no;
	seq_no = LogEvent(llog,ev);
	f_ev = ev;
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"couldn't create second dep on host 1\n");
		exit(1);
	}
	ev = EventCreate(TRIGGER,1);
	ev->reason_host = 1;
	ev->reason_seq_no = seq_no;
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
	ev->reason_host = 1;
	ev->reason_seq_no = seq_no;
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
	LogFree(llog_r);
	LogFree(llog);
	GLogFree(glog_r);
	GLogFree(glog);

	return(0);
}
