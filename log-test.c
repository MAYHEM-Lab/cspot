#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "log.h"

#define ARGS "f:s:"
char *Usage = "log-test -f filename\n\
\t-s size (in events)\n";

char Fname[4096];

int main(int argc, char **argv)
{
	int c;
	int size;
	LOG *llog;
	GLOG *glog;
	EVENT *ev;
	unsigned long long seq_no;
	int i;

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

	llog = LogCreate(Fname,size);
	ev = EventCreate(FUNC,1);
	seq_no = LogEvent(llog,ev);
	EventFree(ev);

	for(i=0; i < size + 2; i++) {
		ev = EventCreate(FUNC,1);
		ev->reason_host = 1;
		ev->reason_seq_no = seq_no;
		seq_no = LogEvent(llog,ev);
		LogPrint(stdout,llog);
		EventFree(ev);
	}
	LogFree(llog);

	return(0);
}
