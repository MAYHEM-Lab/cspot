#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:s:"
char *Usage = "senspot-init -W woof_name\n\
\t-s (history size in number of elements)\n";

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int err;
	SENSPOT spt;
	char wname[4096];
	unsigned long history_size;

	memset(wname,0,sizeof(wname));
	history_size = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 's':
				history_size = atol(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(wname[0] == 0) {
		fprintf(stderr,"must specify filename for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(history_size == 0) {
		fprintf(stderr,"must specify history size for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
	

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	WooFInit();

	err = WooFCreate(wname,sizeof(SENSPOT),history_size);

	if(err < 0) {
		fprintf(stderr,"senspot-init failed for %s with history size %lu\n",
			wname,
			history_size);
		fflush(stderr);
		exit(1);
	}


	exit(0);
}

	

	
	
