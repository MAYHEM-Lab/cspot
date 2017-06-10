#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "log.h"

#define ARGS "f:p:s:S:t:"
char *Usage = "log-test-thread -f filename\n\
\t-p probability of a remote event\n\
\t-s size (in events)\n\
\t-S sync count (number of events before updating log)\n\
\t-t threads (one per host)\n";

#define NAMELEN (1024)

char Fname[NAMELEN];

double Prob;
int Sync_count;
int Threads;
	

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
	EVENT *ev;

	size = 1000;
	Threads = 1;
	Sync_count = 10;
	Prob = 0.1;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
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
			Fname,i);
		strcpy(lnames[i],extent);
		
		memset(extent,0,sizeof(extent));
		sprintf(extent,"%s.%d.globalmio",
			Fname,i);
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
