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
#include "woofc-priv.h"
#include "woofc-caplets.h"

#define ARGS "W:p:"
char *Usage = "woofc-print-cap -W local-woof-name\n\
\t-p permission\n";

int main(int argc, char**argv)
{
	int c;
	char local_woof_name[1024];
	char woof_name[1028];
	int err;
	uint32_t perm = 0;
	WOOF *wf;
	unsigned long seq_no;
	WCAP principal;
	WCAP *new_cap;

	WooFInit();

	memset(local_woof_name,0,sizeof(local_woof_name));
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(local_woof_name,optarg,sizeof(local_woof_name));
				break;
			case 'p':
				perm = atoi(optarg);
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

	if((perm == 0) || (perm > WCAP_MAX_PERM)) {
		fprintf(stderr,"permission must be between 1 and %d\n",
				WCAP_MAX_PERM);
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	sprintf(woof_name,"%s.CAP",local_woof_name);

	wf = WooFOpen(woof_name);
	if(wf == NULL) {
		fprintf(stderr,"could not access %s\n",woof_name);
		exit(1);
	}

	seq_no = WooFLatestSeqno(wf);

	err = WooFReadWithCause(wf,&principal,seq_no,0,0);
	if(err < 0) {
		fprintf(stderr,"could not read principal from %s\n",woof_name);
		WooFDrop(wf);
		exit(1);
	}

	WooFDrop(wf);

	new_cap = WooFCapAttenuate(&principal,perm);
	if(new_cap == NULL) {
		fprintf(stderr,"attenuate failed for %s with perm %d\n",
				woof_name,perm);
		exit(1);
	}

	// sanity check
	if(!WooFCapAuthorized(principal.check,new_cap,perm)) {
		fprintf(stderr,"attenuate failed to auth for %s with perm %d\n",
				woof_name,perm);
		free(new_cap);
		exit(1);
	}

	// try one higher
	if(WooFCapAuthorized(principal.check,new_cap,perm*2)) {
		fprintf(stderr,"attenuate over auth for %s with perm %d\n",
				woof_name,perm);
		free(new_cap);
		exit(1);
	}

	WooFCapPrint(new_cap);
	free(new_cap);


	exit(0);

}
	
