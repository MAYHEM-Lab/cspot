#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define DEBUG


/*
 * handler that gets invoked when a woof put needs a forward
 *
 * looks for a state record with the woof's name and .FWD appended
 *
 * tries the SENSFWD record contains a map from the latest local seq_no to the
 * remote seq_no that replicates it
 */
int senspot_forward(WOOF *wf, unsigned long seq_no, void *ptr)
{
	SENSPOT *spt = (SENSPOT *)ptr;
	SENSPOT fpt;
	SENSFWD sfwd;
	SENSFWDSTATE sync_state;
	char state_woof[4096];
	char sync_woof[4096];
	WOOF *swf;
	unsigned long r_seq_no;
	unsigned long s_seq_no;
	unsigned long curr_seq_no;
	int i;
	int err;
	int pid = getpid();

/*
	fprintf(stdout,"seq_no: %lu %s recv type %c from %s and timestamp %s\n",
			seq_no,
			"Shereen",
			spt->type,
			spt->ip_addr,
			ctime(&spt->tv_sec));
	fflush(stdout);
*/

	/*
	 * create the state woof name from the woof name
	 */
	memset(state_woof,0,sizeof(state_woof));
	strncpy(state_woof,"Shereen",sizeof(state_woof));
	strcat(state_woof,".FWD");
#ifdef DEBUG
	fprintf(stdout,"senspot-forward (%d): woof: %s, seq_no: %lu, forwarding woof: %s\n",
			pid,
			"Shereen",
			seq_no,
			state_woof);
	fflush(stdout);
#endif

	/*
	 * try and open the state woof
	 */
	swf = WooFOpen(state_woof);
	if(swf == NULL) {
		fprintf(stdout,
			"couldn't open state woof %s\n",state_woof);
		fflush(stdout);
		return(0);
	}

	/*
	 * now make sure this is the only forward handler running
	 */
	memset(sync_woof,0,sizeof(sync_woof));
	strncpy(sync_woof,"Shereen",sizeof(state_woof));
	strcat(sync_woof,".FWDSTATE");
	/*
	 * first append put my state in as active
	 */
	sync_state.seq_no = seq_no;
	sync_state.state = FWDACTIVE;
	s_seq_no = WooFPut(sync_woof,NULL,&sync_state);
	if(s_seq_no == (unsigned long)-1) {
		fprintf(stderr,
			"bad put to sync woof %s from seq_no %lu\n",
			sync_woof,
			seq_no);
		fflush(stderr);
		WooFDrop(swf);
		return(0);
	}

	/*
	 * now look at the previous record to make sure that the forwarder is idle
	 */
	err = WooFGet(sync_woof,&sync_state,(s_seq_no-1));
	if(err < 0) {
		fprintf(stderr,
			"error reading sync_woof %s at seq_no %lu\n",
			sync_woof,
			s_seq_no-1);
		fflush(stderr);
		WooFDrop(swf);
		return(0);
	}

	/*
	 * if the system was not IDLE, there is another one running, exit this one
	 *
	 * note that we could spin here but the assumption is another update will be
	 * coming
	 */
	if(sync_state.state != FWDIDLE) {
#ifdef DEBUG
		fprintf(stdout,
		"senspot-forward (%d): forward for state_woof %s %lu is running while system is not idle -- exiting\n",
				pid,state_woof,seq_no);
		fflush(stdout);
#endif
		WooFDrop(swf);
		return(0);
	}
	

	/*
	 * read the last entry
	 */
	i = WooFReadTail(swf,&sfwd,1);

	/*
	 * load up sync state to idle the system on exit
	 */
	sync_state.state = FWDIDLE;
	sync_state.seq_no = seq_no;
	/*
	 * if there are no entries, this there is nothing to do
	 */
	if(i != 1) {
		fprintf(stdout,"no entries in %s\n",state_woof);
		fflush(stdout);
		WooFDrop(swf);
		/*
		 * idle the system
		 */
		WooFPut(sync_woof,NULL,&sync_state);
		return(0);
	}

	/*
	 * if seq_no is 0, then this is the first time for a replicated put.  Start with that
	 */
	if(sfwd.last_local == 0) {
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): state_woof %s is initialized but empty\n",
				pid,state_woof);
		fflush(stdout);
#endif
		/*
		 * forward the data and use the seq_no that comes back as the matching seq_no
		 */
		r_seq_no = WooFPut(sfwd.forward_woof,NULL,ptr);
		if(r_seq_no == (unsigned long)-1) {
			fprintf(stdout,
				"forward from %s to %s failed on first try\n",
					"Shereen",
					sfwd.forward_woof);
			fflush(stdout);
			WooFDrop(swf);
			WooFPut(sync_woof,NULL,&sync_state);
			return(0);
		}
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): successful first put of %lu to %s at %lu\n",
				pid,seq_no,sfwd.forward_woof,r_seq_no);
		fflush(stdout);
#endif
		sfwd.last_local = seq_no;
		sfwd.last_remote = r_seq_no;
		/*
		 * write out the latest
		 */
		s_seq_no = WooFAppend(swf,NULL,&sfwd);
		if(s_seq_no == (unsigned long)-1) {
			fprintf(stdout,
				"append of state to %s failed\n",state_woof);
			fflush(stdout);
		}
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): successful state write to %s of %lu at %lu\n",
				pid,state_woof,seq_no,s_seq_no);
		fflush(stdout);
#endif

		WooFDrop(swf);
		WooFPut(sync_woof,NULL,&sync_state);
		return(1);

	} else { /* try and sync in case there has been a partition */
		/* this seq_no should be one larger than the last one successfully forwarded */
		curr_seq_no = sfwd.last_local+1;
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): synching %s %lu to %s %lu\n",
				pid,"Shereen",seq_no,sfwd.forward_woof,curr_seq_no);
		fflush(stdout);
#endif
		/*
		 * try forwarding the ones that are missing
		 */
		while(curr_seq_no <= (seq_no-1)) {
			err = WooFRead(wf,&fpt,curr_seq_no);
			if(err < 0) {
				fprintf(stdout,
					"bad reaad of %s at %lu on partition\n",
						"Shereen",
						curr_seq_no);
				fflush(stdout);
				WooFDrop(swf);
				WooFPut(sync_woof,NULL,&sync_state);
				return(0);
			}
			r_seq_no = WooFPut(sfwd.forward_woof,NULL,&fpt);
			if(r_seq_no == (unsigned long)-1) {
				fprintf(stdout,
					"bad remote put for %s to %s at %lu local\n",
						"Shereen",
						sfwd.forward_woof,
						curr_seq_no);
				fflush(stdout);
				WooFDrop(swf);
				WooFPut(sync_woof,NULL,&sync_state);
				return(0);
			}
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): successful put of %s %lu to %s %lu\n",
				pid,"Shereen",curr_seq_no,sfwd.forward_woof,r_seq_no);
		fflush(stdout);
#endif
			sfwd.last_local = curr_seq_no;
			sfwd.last_remote = r_seq_no;
			/*
		 	 * write out the latest
		 	 */
			s_seq_no = WooFAppend(swf,NULL,&sfwd);
			if(s_seq_no == (unsigned long)-1) {
				fprintf(stdout,
				 "append of state for %lu to %s failed\n",curr_seq_no,state_woof);
				fflush(stdout);
			}
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): successful state update to %s of %lu at %lu\n",
				pid,state_woof,curr_seq_no,s_seq_no);
		fflush(stdout);
#endif
			curr_seq_no++;
		}
		/*
		 * now forward this seq no
		 */
		r_seq_no = WooFPut(sfwd.forward_woof,NULL,ptr);
		if(r_seq_no == (unsigned long)-1) {
			fprintf(stdout,
				"bad remote put fora current of %s to %s at %lu local\n",
					"Shereen",
					sfwd.forward_woof,
					curr_seq_no);
			fflush(stdout);
			WooFDrop(swf);
			WooFPut(sync_woof,NULL,&sync_state);
			return(0);
		}
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): successful sync put of %s %lu to %s %lu\n",
				pid,"Shereen",seq_no,sfwd.forward_woof,r_seq_no);
		fflush(stdout);
#endif
		sfwd.last_local = seq_no;
		sfwd.last_remote = r_seq_no;
		/*
		 * write out the latest
		 */
		s_seq_no = WooFAppend(swf,NULL,&sfwd);
		if(s_seq_no == (unsigned long)-1) {
			fprintf(stdout,
			 "current append of state for %lu to %s failed\n",curr_seq_no,state_woof);
			fflush(stdout);
		}
#ifdef DEBUG
		fprintf(stdout,
			"senspot-forward (%d): sync wtite to %s of %lu at %lu\n",
				pid,state_woof,seq_no,s_seq_no);
#endif
		WooFDrop(swf);
	} /* end of sync branch */

	WooFPut(sync_woof,NULL,&sync_state);
	return(1);

}

