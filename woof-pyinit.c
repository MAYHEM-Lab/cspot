#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <poll.h>


#include "woofc.h"


LOG *Name_log;
unsigned long Name_id;
char WooF_namespace[2048];
char WooF_dir[2048];
char Host_ip[25];
char WooF_namelog_dir[2048];
char Namelog_name[2048];

int WooFInitEnv() {
	char *wnld;
	char *wf_ns;
	char *wf_dir;
	char *wf_ip;
	char *namelog_name;
	char *name_id;
	MIO *lmio;
	char log_name[4096];

	wf_ns = getenv("WOOFC_NAMESPACE");
	if(wf_ns == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOFC_NAMESPACE\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(WooF_namespace,wf_ns,sizeof(WooF_namespace));

	wf_ip = getenv("WOOF_HOST_IP");
	if(wf_ip == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_HOST_IP\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(Host_ip,wf_ip,sizeof(Host_ip));

	wf_dir = getenv("WOOFC_DIR");
	if(wf_dir == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOFC_DIR\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(WooF_dir,wf_dir,sizeof(WooF_dir));

	wnld = getenv("WOOF_NAMELOG_DIR");
	if(wnld == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_NAMELOG_DIR\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(WooF_namelog_dir,wnld,sizeof(WooF_namelog_dir));

	namelog_name = getenv("WOOF_NAMELOG_NAME");
	if(namelog_name == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_NAMELOG_NAME\n");
		fflush(stderr);
		exit(1);
	}
	strncpy(Namelog_name,namelog_name,sizeof(Namelog_name));

	name_id = getenv("WOOF_NAME_ID");
	if(name_id == NULL) {
		fprintf(stderr,"WooFShepherd: couldn't find WOOF_NAME_ID\n");
		fflush(stderr);
		exit(1);
	}
	Name_id = (unsigned long)atol(name_id);

	sprintf(log_name,"%s/%s",WooF_namelog_dir,namelog_name);
	lmio = MIOReOpen(log_name);
	if(lmio == NULL) {
		fprintf(stderr,
		"WooFShepherd: couldn't open mio for log %s\n",log_name);
		fflush(stderr);
		exit(1);
	}
	Name_log = (LOG *)MIOAddr(lmio);
	strncpy(Namelog_name,namelog_name,sizeof(Namelog_name));
    
    return 0;
}
