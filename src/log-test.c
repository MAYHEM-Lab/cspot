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

char Name2[4096];
char GName2[4096];

char Name3[4096];
char GName3[4096];

int main(int argc, char **argv)
{
	int c;
	int size;
	LOG *llog;
	LOG *llog_2;
	LOG *llog_3;
	GLOG *glog;
	GLOG *glog_2;
	GLOG *glog_3;
	EVENT *ev;
	EVENT *t_ev;
	EVENT *f_ev;
	unsigned long long seq_no;
	int err;
	int i;
	unsigned long long trigger_seq_no;
	unsigned long long trigger_cause_seq_no;
	unsigned long long last_host_1;
	unsigned long long last_host_2;
	unsigned long long last_host_3;

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
	strcpy(Name2,"remote.");
	strcat(Name2,Fname);
	strcpy(GName2,"remoteglobal.");
	strcat(GName2,Fname);
	strcpy(Name3,"third.");
	strcat(Name3,Fname);
	strcpy(GName3,"thirdglobal.");
	strcat(GName3,Fname);

	llog = LogCreate(Fname,1,size);
	if(llog == NULL) {
		fprintf(stderr,"couldn't create local log %s\n",
			Fname);
		fflush(stderr);
		exit(1);
	}

	glog = GLogCreate(Gname,1,size);
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

	llog_2 = LogCreate(Name2,2,size);
	if(llog_2 == NULL) {
		fprintf(stderr,"couldn't create remote local log\n");
		exit(1);
	}

	glog_2 = GLogCreate(GName2,2,size);
	if(glog_2 == NULL) {
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
	seq_no = LogEvent(llog_2,ev);
	err = GLogEvent(glog_2,ev);	/* resolve immediately */
					/* first time */
	if(err < 0) {
		fprintf(stderr,"first dependency event failed\n");
		exit(1);
	}
	EventFree(ev);
	GLogPrint(stdout,glog_2);

	/*
	 * now, later, TRIGGER event arrives from host 1
	 */
	ev = EventCreate(TRIGGER,1);
	ev->seq_no = trigger_seq_no;
	ev->cause_seq_no = trigger_cause_seq_no;
	ev->cause_host = 1;
	err = GLogEvent(glog_2,ev);		/* should do nothing */
	if(err < 0) {
		fprintf(stderr,"clear dependency failed on host 2\n");
		exit(1);
	}
	EventFree(ev);
	GLogPrint(stdout,glog_2);

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
	seq_no = LogEvent(llog_2,ev);
	err = GLogEvent(glog_2,ev);
	if(err < 0) {
		fprintf(stderr,"error logging triggerd function on host 2\n");
		exit(1);
	}
	EventFree(ev);

	/*
	 * later TRIGGER arrives from host 1 -- should create dependency
	 */
	err = GLogEvent(glog_2,t_ev);
	GLogPrint(stdout,glog_2);
	if(err < 0) {
		fprintf(stderr,"error logging later trigger arrival\n");
		exit(1);
	}
		

	/*
	 * and then the dependency for the trigger arrives.  This should 
	 * clear the dependencies on host 2
	 */
	err = GLogEvent(glog_2,f_ev);
	if(err < 0) {
		fprintf(stderr,
			"error logging second function arrival at host 2\n");
		exit(1);
	}
	GLogPrint(stdout,glog_2);

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
	EventFree(ev);
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	err = GLogEvent(glog,ev);
	if(err < 0) {
		fprintf(stderr,"could log second unrelated event on host 1\n");
		fflush(stderr);
	}
	EventFree(ev);
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
	EventFree(ev);

	/*
	 * now try to import host 1 log on host 2
	 */
	err = ImportLogTail(glog_2,llog);
	if(err < 0) {
		fprintf(stderr,"first host 2 import failed\n");
		exit(1);
	}
	GLogPrint(stdout,glog_2);

	/*
	 * now try and introduce a new host and a have it participate in a
	 * chain of events
	 */
	llog_3 = LogCreate(Name3,3,size);
	if(llog_3 == NULL) {
		fprintf(stderr,"couldn't create third log\n");
		fflush(stderr);
		exit(1);
	}
	glog_3 = GLogCreate(GName3,3,size);
	if(glog_3 == NULL) {
		fprintf(stderr,"couldn't create third global log\n");
		fflush(stderr);
		exit(1);
	}
	/*
	 * on host 1
	 */
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	GLogEvent(glog,ev);
	EventFree(ev);
	ev = EventCreate(TRIGGER,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	GLogEvent(glog,ev);
	EventFree(ev);
	/*
	 * fires on host 2
	 */
	ev = EventCreate(TRIGGER,2);
	ev->cause_host = 1;
	ev->cause_seq_no = seq_no;
	last_host_2 = seq_no = LogEvent(llog_2,ev);
	GLogEvent(glog_2,ev);
	EventFree(ev);
	ev = EventCreate(FUNC,2);
	ev->cause_host = 2;
	ev->cause_seq_no = last_host_2;
	last_host_2 = seq_no = LogEvent(llog_2,ev);
	GLogEvent(glog_2,ev);
	EventFree(ev);
	ev = EventCreate(TRIGGER,2);
	ev->cause_host = 2;
	ev->cause_seq_no = last_host_2;
	last_host_2 = seq_no = LogEvent(llog_2,ev);
	GLogEvent(glog_2,ev);
	EventFree(ev);
	/*
	 * fires on host 3
	 */
	ev = EventCreate(TRIGGER,3);
	ev->cause_host = 2;
	ev->cause_seq_no = seq_no;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	GLogEvent(glog_3,ev);
	EventFree(ev);
	ev = EventCreate(FUNC,3);
	ev->cause_host = 3;
	ev->cause_seq_no = last_host_3;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	GLogEvent(glog_3,ev);
	EventFree(ev);
	ev = EventCreate(TRIGGER,3);
	ev->cause_host = 3;
	ev->cause_seq_no = last_host_3;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	GLogEvent(glog_3,ev);
	EventFree(ev);
	/*
	 * fires on host 1
	 */
	ev = EventCreate(TRIGGER,1);
	ev->cause_host = 3;
	ev->cause_seq_no = seq_no;
	last_host_1 = seq_no = LogEvent(llog,ev);
	GLogEvent(glog,ev);
	EventFree(ev);
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no = LogEvent(llog,ev);
	GLogEvent(glog,ev);
	EventFree(ev);

	printf("LOG 1\n");
	GLogPrint(stdout,glog);
	printf("LOG 2\n");
	GLogPrint(stdout,glog_2);
	printf("LOG 3\n");
	GLogPrint(stdout,glog_3);

	/*
	 * now import host 2's entries to host 1
	 */
	err = ImportLogTail(glog,llog_2);
	if(err < 0) {
		fprintf(stderr,
			"trouble importing host 2 local log to host 1\n");
		fflush(stderr);
		exit(1);
	}
	printf("LOG 1 after import from 2\n");
	GLogPrint(stdout,glog);

	/*
	 * and vice versa
	 */
	err = ImportLogTail(glog_2,llog);
	if(err < 0) {
		fprintf(stderr,
			"trouble importing host 1 local log to host 2\n");
		fflush(stderr);
		exit(1);
	}
	printf("LOG 2 after import from 1\n");
	GLogPrint(stdout,glog_2);

	/*
	 * now merge host 2 with host 3
	 */
	err = ImportLogTail(glog_3,llog_2);
	if(err < 0) {
		fprintf(stderr,
			"trouble importing host 2 local log to host 3\n");
		fflush(stderr);
		exit(1);
	}
	printf("LOG 3 after import from 2\n");
	GLogPrint(stdout,glog_3);

	err = ImportLogTail(glog_2,llog_3);
	if(err < 0) {
		fprintf(stderr,
			"trouble importing host 3 local log to host 2\n");
		fflush(stderr);
		exit(1);
	}
	printf("LOG 2 after import from 3\n");
	GLogPrint(stdout,glog_2);
	

	LogFree(llog_2);
	LogFree(llog);
	GLogFree(glog_2);
	GLogFree(glog);

	/*
	 * now try for a total order
	 */
	llog = LogCreate(Fname,1,size);
	if(llog == NULL) {
		fprintf(stderr,"couldn't recreate host 1 log\n");
		exit(1);
	}
	glog = GLogCreate(Gname,1,size);
	if(glog == NULL) {
		fprintf(stderr,"couldn't create global log 1\n");
		exit(1);
	}

	llog_2 = LogCreate(Name2,2,size);
	if(llog_2 == NULL) {
		fprintf(stderr,"couldn't recreate host 2 log\n");
		exit(1);
	}
	glog_2 = GLogCreate(GName2,2,size);
	if(glog_2 == NULL) {
		fprintf(stderr,"couldn't create global log 2\n");
		exit(1);
	}

	llog_3 = LogCreate(Name3,3,size);
	if(llog_3 == NULL) {
		fprintf(stderr,"couldn't recreate host 3 log\n");
		exit(1);
	}
	glog_3 = GLogCreate(GName3,3,size);
	if(glog_2 == NULL) {
		fprintf(stderr,"couldn't create global log 3\n");
		exit(1);
	}
	
	/*
	 * prime the pump with one function on each
	 */
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = 0;
	last_host_1 = seq_no = LogEvent(llog,ev);
	EventFree(ev);

	ev = EventCreate(FUNC,2);
	ev->cause_host = 2;
	ev->cause_seq_no = 0;
	last_host_2 = seq_no = LogEvent(llog_2,ev);
	EventFree(ev);

	ev = EventCreate(FUNC,3);
	ev->cause_host = 3;
	ev->cause_seq_no = 0;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	EventFree(ev);

	/*
	 * merge them all on host 1
	 */
	ImportLogTail(glog,llog);
	ImportLogTail(glog,llog_2);
	ImportLogTail(glog,llog_3);
	GLogPrint(stdout,glog);

	/*
	 * now make up some events on various hosts
	 */
	ev = EventCreate(FUNC,3);
	ev->cause_host = 3;
	ev->cause_seq_no = last_host_3;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	EventFree(ev);
	ev = EventCreate(FUNC,3);
	ev->cause_host = 3;
	ev->cause_seq_no = last_host_3;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	EventFree(ev);
	ev = EventCreate(FUNC,3);
	ev->cause_host = 3;
	ev->cause_seq_no = last_host_3;
	last_host_3 = seq_no = LogEvent(llog_3,ev);
	EventFree(ev);

	/*
	 * fire on 2
	 */
	ev = EventCreate(FUNC,2);
	ev->cause_host = 3;
	ev->cause_seq_no = last_host_3;
	seq_no = LogEvent(llog_2,ev);
	EventFree(ev);

	/*
	 * unrealted to host 3
	 */
	ev = EventCreate(FUNC,2);
	ev->cause_host = 2;
	ev->cause_seq_no = last_host_2;
	last_host_2 = seq_no  = LogEvent(llog_2,ev);
	EventFree(ev);

	ev = EventCreate(FUNC,2);
	ev->cause_host = 2;
	ev->cause_seq_no = last_host_2;
	last_host_2 = seq_no  = LogEvent(llog_2,ev);
	EventFree(ev);

	ev = EventCreate(FUNC,2);
	ev->cause_host = 2;
	ev->cause_seq_no = last_host_2;
	seq_no  = LogEvent(llog_2,ev);
	EventFree(ev);

	/*
	 * and on host 1
	 */
	ev = EventCreate(FUNC,1);
	ev->cause_host = 2;
	ev->cause_seq_no = last_host_2;
	seq_no  = LogEvent(llog,ev);
	EventFree(ev);

	/*
	 * unrelated to host 2
	 */
	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no  = LogEvent(llog,ev);
	EventFree(ev);

	ev = EventCreate(FUNC,1);
	ev->cause_host = 1;
	ev->cause_seq_no = last_host_1;
	last_host_1 = seq_no  = LogEvent(llog,ev);
	EventFree(ev);

	/*
	 * merge all on host 1
	 */
	ImportLogTail(glog,llog);
	ImportLogTail(glog,llog_2);
	ImportLogTail(glog,llog_3);
	GLogPrint(stdout,glog);

	/*
	 * now do the same merge on host 2 in different order
	 */
	ImportLogTail(glog_2,llog_2);
	ImportLogTail(glog_2,llog);
	ImportLogTail(glog_2,llog_3);
	GLogPrint(stdout,glog_2);

	/*
	 * and on host 3
	 */
	ImportLogTail(glog_3,llog);
	ImportLogTail(glog_3,llog_3);
	ImportLogTail(glog_3,llog_2);
	GLogPrint(stdout,glog_3);

	LogFree(llog);
	LogFree(llog_2);
	LogFree(llog_3);
	GLogFree(glog);
	GLogFree(glog_2);
	GLogFree(glog_3);


	return(0);
}
