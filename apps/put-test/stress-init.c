#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "put-test.h"

#define ARGS "c:W:s:N:H:"
char *Usage = "stress_init -W woof_name for stress test\n\
\t-H namelog-path\n\
\t-N target namespace (as a URI)\n\
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

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 'N':
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			case 'H':
				strncpy(Namelog_dir,optarg,sizeof(Namelog_dir));
				break;
			case 's':
				woof_size = atoi(optarg);
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

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	memset(local_ns,0,sizeof(local_ns));
	err = WooFURINameSpace(NameSpace,local_ns,sizeof(local_ns));
	if(err < 0) {
		fprintf(stderr,"must specify namespace for experiment as URI\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	sprintf(putbuf1,"WOOFC_DIR=%s",local_ns);
	putenv(putbuf1);

	WooFInit();

	MAKE_EXTENDED_NAME(Iname,Wname,"input");
	MAKE_EXTENDED_NAME(Oname,Wname,"output");


	/*
	 * create a local woof to hold time stamps for stress test
	 */
	err = WooFCreate(Iname,sizeof(ST_EL),woof_size);
	if(err < 0) {
		fprintf(stderr,"stress-init: can't init %s\n",Iname);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(Oname,sizeof(ST_EL),woof_size);
	if(err < 0) {
		fprintf(stderr,"stress-init: can't init %s\n",Oname);
		fflush(stderr);
		exit(1);
	}

	return(1);
}


