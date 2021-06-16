#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "put-test.h"

#define ARGS "c:f:s:N:H:"
char *Usage = "recv_init -f woof_name for recv experiment\n\
\t-H namelog-path\n\
\t-N target namespace (as a URI)\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_SIMULTANEOUS_EXP (10)

int main(int argc, char **argv)
{
	int c;
	int err;
	char local_ns[1024];

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'N':
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			case 'H':
				strncpy(Namelog_dir,optarg,sizeof(Namelog_dir));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for experiment\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

#if 0
	if(NameSpace[0] == 0) {
		fprintf(stderr,"must specify namespace for experiment\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
#endif

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

#if 0
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
#endif

	WooFInit();


	sprintf(Wname,"%s/%s",NameSpace,Fname);

	/*
	 * create a local woof to hold the args for experiments
	 */
	err = WooFCreate(Fname,sizeof(PT_EL),MAX_SIMULTANEOUS_EXP);
	if(err < 0) {
		fprintf(stderr,"recv-init: can't init %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	return(1);
}


