#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/hmac.h>
#include <time.h>
#include <sys/time.h>

#include "woofc-access.h"
#include "woofc.h"
#include "woofc-caplets.h"

#define ARGS "W:"
char *Usage = "woofc-init-principal -W local-woof-name\n";

int main(int argc, char**argv)
{
	int c;
	char local_woof_name[1024];
	int err;

	WooFInit();

	memset(local_woof_name,0,sizeof(local_woof_name));
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(local_woof_name,optarg,sizeof(local_woof_name));
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(local_woof_name[0] == 0) {
		fprintf(stderr,"must specify local woof name\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	err = WooFCapInit(local_woof_name);
	if(err < 0) {
		fprintf(stderr,"principal cap init failed for %s\n",local_woof_name);
		exit(1);
	}
	exit(0);

}
	
