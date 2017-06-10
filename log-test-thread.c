#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include "log.h"

#define ARGS "f:s:"
char *Usage = "log-test-thread -f filename\n\
\t-s size (in events)\n";

char Name1[4096];
char Gname1[4096];

char Name2[4096];
char GName2[4096];

char Name3[4096];
char GName3[4096];

	

int main(int argc, char **argv)
{
	int c;
	int size;
	LOG *llog_1;
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


	return(0);
}
