#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj1.h"

#define ARGS "c:f:s:"
char *Usage = "woofc-obj1-test -f filename -s size (in events)\n";

char Fname[4096];

int main(int argc, char **argv)
{
	int c;
	int size;
	int err;
	OBJ1_EL el;

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
		fprintf(stderr,"must specify filename for object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit(1);


	err = WooFCreate(Fname,sizeof(OBJ1_EL),size);

	if(err < 0) {
		fprintf(stderr,"couldn't create wf_1\n");
		fflush(stderr);
		exit(1);
	}

	memset(el.string,0,sizeof(el.string));
	strncpy(el.string,"my first bark",sizeof(el.string));

	err = WooFPut(Fname,"woofc_obj1_handler_1",(void *)&el);

	if(err < 0) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return(0);
}

