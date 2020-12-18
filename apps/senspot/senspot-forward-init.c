#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:F:"
char *Usage = "senspot-forward-init -W local_state_woof_name\n\
\t-F woof_to_receive_forward\n";

char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20
#define STATE_SIZE (2)

/*
 * creates a state woof for use by senspot_forward run as a handler
 *
 * checks to see if there is a local woof to replicate before creating
 * woof to keep the seq numbers (state woof)
 *
 * does not check to ssee if there is a remote woof with the name that is specified
 */

int main(int argc, char **argv)
{
	int c;
	int err;
	SENSFWD sfwd;
	SENSFWDSTATE sync_state;
	char wname[4096];
	char fname[4096];
	char sname[4096];
	char *s;
	unsigned long seq_no;
	WOOF *wf;

	memset(wname,0,sizeof(wname));
	memset(fname,0,sizeof(fname));

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'F':
				strncpy(fname,optarg,sizeof(wname));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(wname[0] == 0) {
		fprintf(stderr,"must specify filename local state woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(fname[0] == 0) {
		fprintf(stderr,"must specify woof path for remote woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	WooFInit();

	/*
	 * see if there is a local woof that we can forward.  If not, error
	 */
	wf = WooFOpen(wname);
	if(wf == NULL) {
		fprintf(stderr,
			"couldn't open %s\n",wname);
		fflush(stderr);
		exit(1);
	}
	WooFDrop(wf);

	/*
	 * create the local sync woof so that only one forward runs at a time
	 */
	strncpy(sname,wname,sizeof(sname));
	s = strcat(sname,".FWDSTATE");
	if(s == NULL) {
		fprintf(stderr,
			"strcat of .FWDSTATE to %s failed\n",wname);
		fflush(stderr);
		exit(1);
	}
	err = WooFCreate(sname,sizeof(SENSFWDSTATE),STATE_SIZE);

	if(err < 0) {
		fprintf(stderr,"senspot-forward-init failed for %s with history size %lu\n",
			sname,
			STATE_SIZE);
		fflush(stderr);
		exit(1);
	}

	/*
	 * start the system in an IDLE state
	 */
	memset(&sync_state,0,sizeof(SENSFWDSTATE));
	sync_state.state = FWDIDLE;
	seq_no = WooFPut(sname,NULL,&sync_state);
	if(seq_no == (unsigned long)-1) {
		fprintf(stderr,
			"senspot-forward-init could write IDLE state\n");
		fflush(stderr);
		exit(1);
	}

	/*
	 * create the local mapping woof
	 */
	s = strcat(wname,".FWD");
	if(s == NULL) {
		fprintf(stderr,
			"strcat of .FWD to %s failed\n",wname);
		fflush(stderr);
		exit(1);
	}
	err = WooFCreate(wname,sizeof(SENSFWD),STATE_SIZE);

	if(err < 0) {
		fprintf(stderr,"senspot-forward-init failed for %s with history size %lu\n",
			wname,
			STATE_SIZE);
		fflush(stderr);
		exit(1);
	}

	/*
	 * copy remote woof name into local state woof
	 */
	memset(&sfwd,0,sizeof(SENSFWD));
	strncpy(sfwd.forward_woof,fname,sizeof(sfwd.forward_woof));

	seq_no = WooFPut(wname,NULL,&sfwd);

	if(seq_no == (unsigned long)-1) {
		fprintf(stderr,
			"couldn't write FWD entry to %s\n",wname);
		fflush(stderr);
		exit(1);
	}
	
	exit(0);
}

