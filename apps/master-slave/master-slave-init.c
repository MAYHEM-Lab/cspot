#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "master-slave.h"

#define ARGS "W:M:S:C:I:s:"
char *Usage = "master-slave-init -W woof-name\n\
\t-M master-ip-addr\n\
\t-S slave-ip-addr\n\
\t-C client-ip-addr\n\
\t-I my-state <master/slave/client>\n\
\t-s (history size in number of elements)\n";

char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

extern char WooF_dir[2048];

int main(int argc, char **argv)
{
	int c;
	int err;
	char wname[4096];
	char ping_woof[4096];
	char status_woof[4096];
	char pulse_woof[4096];
	char master_ip[IPSTRLEN+1];
	char slave_ip[IPSTRLEN+1];
	char client_ip[IPSTRLEN+1];
	STATE state;
	STATUS status;
	char start_state[255];
	unsigned long seq_no;

	unsigned long history_size;

	history_size = 0;
	memset(wname,0,sizeof(wname));
	memset(master_ip,0,sizeof(master_ip));
	memset(slave_ip,0,sizeof(slave_ip));
	memset(client_ip,0,sizeof(client_ip));
	memset(start_state,0,sizeof(start_state));

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy((char *)wname,optarg,sizeof(wname));
				break;
			case 's':
				history_size = atol(optarg);
				break;
			case 'M':
				strncpy(master_ip,optarg,sizeof(master_ip));
				break;
			case 'S':
				strncpy(slave_ip,optarg,sizeof(slave_ip));
				break;
			case 'C':
				strncpy(client_ip,optarg,sizeof(client_ip));
				break;
			case 'I':
				strncpy(start_state,optarg,sizeof(start_state));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(wname[0] == 0) {
		fprintf(stderr,"must specify filename for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(master_ip[0] == 0) {
		fprintf(stderr,"must specify ip address of master\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(slave_ip[0] == 0) {
		fprintf(stderr,"must specify ip address of slave\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(client_ip[0] == 0) {
		fprintf(stderr,"must specify ip address of client\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(start_state[0] == 0) {
		fprintf(stderr,"must specify start state\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if((strcmp(start_state,"master") != 0) &&
            (strcmp(start_state,"slave") != 0) &&
	    (strcmp(start_state,"client") != 0)) {
		fprintf(stderr,"unrecognized start state\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(history_size == 0) {
		fprintf(stderr,"must specify history size for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}


	memset(ping_woof,0,sizeof(ping_woof));
	memset(status_woof,0,sizeof(status_woof));
	MAKE_EXTENDED_NAME(ping_woof,wname,"ping");
	MAKE_EXTENDED_NAME(status_woof,wname,"status");
	MAKE_EXTENDED_NAME(pulse_woof,wname,"pulse");

	WooFInit();

	/*
	 * first create the woofs to hold the data
	 */
	err = WooFCreate(wname,sizeof(STATE),history_size); 
	if(err < 0) {
		fprintf(stderr,"master-slave-init failed for %s with history size %lu\n",
			wname,
			history_size);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(status_woof,sizeof(STATUS),history_size);
	if(err < 0) {
		fprintf(stderr,"master-slave-init failed for %s with history size %lu\n",
			status_woof,
			history_size);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(pulse_woof,sizeof(PULSE),history_size);
	if(err < 0) {
		fprintf(stderr,"master-slave-init failed for %s with history size %lu\n",
			pulse_woof,
			history_size);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(ping_woof,sizeof(PINGPONG),history_size);
	if(err < 0) {
		fprintf(stderr,"master-slave-init failed for %s with history size %lu\n",
			ping_woof,
			history_size);
		fflush(stderr);
		exit(1);
	}

	/* start status out with local information about remote side */
	if(strcmp(start_state,"master") == 0) {
		status.local = 'M'; /* from perspective of other side */
		status.remote = 'S';
	} else if(strcmp(start_state,"slave") == 0) {
		status.local = 'S';
		status.remote = 'M';
	} else {
		status.local = 'C';
		status.remote = 'C';
	}

	/* assume other wide starts at 1 */
	status.remote_seq_no = 1;

	seq_no = WooFPut(status_woof,NULL,(void *)&status);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"status put failed to %s\n",status_woof);
		fflush(stderr);
	}

	memset(&state,0,sizeof(state));
	/*
	 * make fully qualified woof name
	 */
	strncpy(state.wname,wname,sizeof(state.wname));
	if(status.local == 'M') {
		strncpy(state.my_ip,master_ip,sizeof(state.my_ip));
		state.my_state = 'M';
		strncpy(state.other_ip,slave_ip,sizeof(state.my_ip));
		state.other_state = 'S';
		sprintf(state.wname,"woof://%s/%s/%s",state.my_ip,WooF_dir,wname);
	} else if(status.local == 'S') {
		strncpy(state.my_ip,slave_ip,sizeof(state.my_ip));
		state.my_state = 'S';
		strncpy(state.other_ip,master_ip,sizeof(state.my_ip));
		state.other_state = 'M';
		sprintf(state.wname,"woof://%s/%s/%s",state.my_ip,WooF_dir,wname);
	} else {
		/* use master as "my" for client */
		strncpy(state.client_ip,client_ip,sizeof(state.client_ip));
		strncpy(state.my_ip,master_ip,sizeof(state.my_ip));
		strncpy(state.other_ip,slave_ip,sizeof(state.my_ip));
		sprintf(state.wname,"woof://%s/%s/%s",state.client_ip,WooF_dir,wname);
		state.my_state = 'C';
	}
	strncpy(state.client_ip,client_ip,sizeof(state.client_ip));
	
	state.other_color = 'G';
	state.client_color = 'G';

	seq_no = WooFPut(wname,NULL,(void *)&state);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"state put failed to %s\n",wname);
		fflush(stderr);
	}
		
	exit(0);
}

	
