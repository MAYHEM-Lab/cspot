#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "cspot-app-example.h"

#define ARGS "W:s:"
char *Usage = "cspot-app-example-init -W woof_name \n\
\t-s woof_size (in number of elements)\n";

char Wname[4096];
char Iname[4096];
char Oname[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	int woof_size = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
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


	MAKE_EXTENDED_NAME(Iname,Wname,"input");
	MAKE_EXTENDED_NAME(Oname,Wname,"output");

	if(!WoofValidURI(Iname)) {
		WooFInit();
	}

	/*
	 * create an input woof for the handler to read
	 */
	err = WooFCreate(Iname,sizeof(EX_EL),woof_size);
	if(err < 0) {
		fprintf(stderr,"cspot-app-example-init: can't init %s\n",Iname);
		fflush(stderr);
		exit(1);
	}

	/*
	 * create an output woof for the handler to write
	 */
	err = WooFCreate(Oname,sizeof(EX_EL),woof_size);
	if(err < 0) {
		fprintf(stderr,"cspot-example-init: can't init %s\n",Oname);
		fflush(stderr);
		exit(1);
	}

	return(1);
}


