#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj2.h"

#define ARGS "c:f:s:"
char *Usage = "woofc-obj2-test -f filename -s size (in events)\n";

char Fname[4096];

int main(int argc, char **argv)
{
	int c;
	int size;
	int err;
	OBJ2_EL el;

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


	err = WooFCreate(Fname,sizeof(OBJ2_EL),size);

	if(err < 0) {
		fprintf(stderr,"couldn't create wf_1\n");
		fflush(stderr);
		exit(1);
	}


	el.counter = 0;
	err = WooFPut(Fname,"woofc_obj2_handler_2",(void *)&el);

	if(err < 0) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return(0);
}

