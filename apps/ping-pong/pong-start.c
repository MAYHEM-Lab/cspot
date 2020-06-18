#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "ping-pong.h"

#define ARGS "W:s:"
char *Usage = "pong-start -W woof_name\n\
\t-s size\n";

char Wname[4096];
char NameSpace[4096];
char putbuf1[4098];
char putbuf2[4098];

int main(int argc, char **argv)
{
	int c;
	int err;
	PP_EL el;
	unsigned long seq_no;
	unsigned int pid;
	char local_ns[4096];
	char l_wname[4096];
	int size;

	size = 5;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
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

	if(Wname[0] == 0) {
		fprintf(stderr,"must specify fully qualified woof name for pong object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	/*
	 * I am pong
	 */
	memset(local_ns,0,sizeof(local_ns));
	err = WooFNameSpaceFromURI(Wname,local_ns,sizeof(local_ns));
	if(err < 0) { // not a valid woof spec
		fprintf(stderr,"%s does not contain a valid namespace\n",Wname);
		fprintf(stderr,"please specify full woof name for pong\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	} else { // name space okay
		sprintf(putbuf1,"WOOFC_DIR=%s",local_ns);
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",local_ns);
	}
	putenv(putbuf1);

	WooFInit();

	memset(l_wname,0,sizeof(l_wname));
	err = WooFNameFromURI(Wname,l_wname,sizeof(l_wname));
	if(err < 0) {
		fprintf(stderr,"%s does not contain a valid object name for pong\n",
			Wname);
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	/*
	 * create pong's woof and exit -- ping goes first
	 */
	err = WooFCreate(l_wname,sizeof(PP_EL),size);

	if(err < 0) {
		fprintf(stderr,"couldn't create %s in %s from %s\n",
			l_wname,
			NameSpace,
			Wname);
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return(0);
}

