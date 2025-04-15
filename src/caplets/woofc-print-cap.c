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

#define ARGS "W:Np:h:C:P:"
char *Usage = "woofc-print-cap [-W local-woof-name || -N <for namespace>]\n\
\t-C check from an existing cap (will use principal if zero)\n\
\t-P permissions from an existing cap\n\
\t-p permission\n\
\t-h host_ip or host_name\n";

int main(int argc, char**argv)
{
	int c;
	char local_woof_name[1024];
	char woof_name[1028];
	char host_ip[1024];
	char *cwd;
	char pname[1024];
	int err;
	uint32_t perm = 0;
	WOOF *wf;
	unsigned long seq_no;
	WCAP principal;
	WCAP *new_cap;
	uint64_t check = 0;
	uint32_t start_perm = 0;
	int is_namespace;

	WooFInit();

	memset(host_ip,0,sizeof(host_ip));
	memset(local_woof_name,0,sizeof(local_woof_name));
	is_namespace = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(local_woof_name,optarg,sizeof(local_woof_name));
				break;
			case 'N':
				is_namespace = 1;
				break;
			case 'C':
				check = (uint64_t)strtol(optarg,NULL,10);
				break;
			case 'p':
				perm = atoi(optarg);
				break;
			case 'P':
				start_perm = (uint32_t)strtol(optarg,NULL,16);
				break;
			case 'h':
				strncpy(host_ip,optarg,sizeof(host_ip));
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if((is_namespace == 0) && (local_woof_name[0] == 0)) {
		fprintf(stderr,"must specify local woof name or namepsace path\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if((perm == 0) || (perm > WCAP_MAX_PERM)) {
		fprintf(stderr,"permission must be between 1 and %d\n",
				WCAP_MAX_PERM);
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(check == 0) { // start with principal cap
		if(is_namespace == 1) {
			sprintf(woof_name,"%s/CSPOT.CAP",local_woof_name);
		} else {
			sprintf(woof_name,"%s.CAP",local_woof_name);
		}

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
	} else { // create cap to attenuate from check
		if(perm > start_perm) {
			fprintf(stderr,"can't attenuate perm %x up to %x\n",
					perm, start_perm);
			exit(1);
		}
		memset(&principal,0,sizeof(principal));
		principal.check = check;
		principal.permissions = start_perm;
	}

	new_cap = WooFCapAttenuate(&principal,perm);
	if(new_cap == NULL) {
		fprintf(stderr,"attenuate failed for %s with perm %d\n",
				woof_name,perm);
		exit(1);
	}

	// sanity check if we start from the principal
	if(check == 0) {
		if(!WooFCapAuthorized(principal.check,new_cap,perm)) {
			fprintf(stderr,
				"attenuate failed to auth for %s with perm %d\n",
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
	}

	cwd = getcwd(NULL,0);
	if(cwd == NULL) {
		perror("cwd failed ");
		free(new_cap);
		exit(1);
	}

	if(host_ip[0] != 0) {
		if(is_namespace == 1) {
			sprintf(pname,"woof://%s%s",host_ip,cwd);
		} else {
			sprintf(pname,"woof://%s%s/%s",host_ip,cwd,local_woof_name);
		}
	} else {
		if(is_namespace == 1) {
			sprintf(pname,"%s",cwd);
		} else {
			sprintf(pname,"%s/%s",cwd,local_woof_name);
		}
	}
	free(cwd);

	if(is_namespace == 1) {
		WooFNamespaceCapPrint(pname,new_cap);
	} else {
		WooFCapPrint(pname,new_cap);
	}
	free(new_cap);


	exit(0);

}
	
