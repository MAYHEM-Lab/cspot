#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj2.h"

#define ARGS "c:f:s:N:"
char *Usage = "woofc-obj2-test -f filename -s size (in events) -N namespace-path\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
int UseNameSpace;

int main(int argc, char **argv)
{
	int c;
	int size;
	int err;
	OBJ2_EL el;
	unsigned long ndx;
	char putbuf[4096];

	size = 5;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 'N':
				UseNameSpace = 1;
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(UseNameSpace == 1) {
		sprintf(putbuf,"WOOFC_DIR=%s",NameSpace);
		putenv(putbuf);
		sprintf(Wname,"woof://%s/%s",NameSpace,Fname);
	} else {
		strncpy(Wname,Fname,sizeof(Wname));
	}

	WooFInit();


	err = WooFCreate(Wname,sizeof(OBJ2_EL),size);

	if(err < 0) {
		fprintf(stderr,"couldn't create wf_1 at %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	memset(&el,0,sizeof(el));

	if(UseNameSpace == 1) {
		strncpy(el.next_woof,Wname,sizeof(el.next_woof));
	}


	el.counter = 0;
	ndx = WooFPut(Fname,"woofc_obj2_handler_1",(void *)&el);

	if(WooFInvalid(ndx)) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return(0);
}

