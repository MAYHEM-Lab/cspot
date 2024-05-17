#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "stress-test.h"

#define ARGS "P:W:s:"
char *Usage = "stress_init -W local_woof_name for stress test\n\
\t-P payload size\n\
\t-s woof size\n";

char Wname[4096];
char Iname[4096];
char Oname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	char local_ns[1024];
	int woof_size = 0;
	int payload_size = sizeof(ST_EL);
	int local = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 's':
				woof_size = atoi(optarg);
				break;
			case 'P':
				payload_size = atoi(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wname[0] == 0) {
		fprintf(stderr,"must specify woof name for experiment\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(woof_size == 0){
		fprintf(stderr,"need to specify woof size\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}


	WooFInit();

	MAKE_EXTENDED_NAME(Iname,Wname,"input");
	MAKE_EXTENDED_NAME(Oname,Wname,"output");

	if(payload_size < sizeof(ST_EL)) {
		payload_size = sizeof(ST_EL);
	}


	/*
	 * create a local woof to hold time stamps for stress test
	 */
	err = WooFCreate(Iname,payload_size,woof_size);
	if(err < 0) {
		fprintf(stderr,"stress-init: can't init %s\n",Iname);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(Oname,payload_size,woof_size);
	if(err < 0) {
		fprintf(stderr,"stress-init: can't init %s\n",Oname);
		fflush(stderr);
		exit(1);
	}

	return(1);
}


