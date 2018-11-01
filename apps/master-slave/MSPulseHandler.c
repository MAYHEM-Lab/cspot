#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "mastr-slave.h"

int PingPongTest(STATE *state, char target)
{
	PINGPONG l_pp;
	PINGPONG g_pp;
	WOOF *l_pp_w;
	char *pp_woof[4096];
	char l_name[255];
	char l_pp_name[256];
	char r_pp_woof[4096];
	int err;
	unsigned long p_seq_no;
	unsigned long g_seq_no;
	unsigned long curr_seq_no;
	int retry_count;
	int found;

	err = WooFNameFromURI(state.wname,l_name,sizeof(l_name));
	if(err < 0) {
		fprintf(stderr,
			"PingPongTest: couldn't extract local name from %s\n",
				state.wname);
		fflush(stderr);
		return(-1);
	}
	MAKE_EXTENDED_NAME(&l_pp.return_woof,wname,"pingpong");
	MAKE_EXTENDED_NAME(l_pp_name,l_name,"pingpong");

	/* make remore pingpong woof name */
	if(target == 'O') {
		sprintf(r_pp_woof,"woof://%s/%s",state.other_ip,l_pp_name);
	} else if(target == 'C') {
		sprintf(r_pp_woof,"woof://%s/%s",state.client_ip,l_pp_name);
	} else {
		fprintf(stderr,
			"PingPongTest: unrecognized target %s\n",target);
		fflush(stderr);
		return(-1);
	}

	/* open the local pinpong woof */
	l_pp_w = WooFOpen(l_pp_name);
	if(l_pp_w == NULL) {
		fprintf(stderr,
			"PingPongTest: couldn't open local pp woof %s\n",
				l_pp_name);
		fflushg(stderr);
		return(-1);
	}

	/* write current seq_no into record */
	l_pp.seq_no = WooFLatestSeqno(l_pp_w);

	/* put it and expect a return */
	p_seq_no = WooFPut(r_pp_woof,"MSPingPongHandler",(void *)&l_pp);
	if(WooFInvalid(p_seq_no)) {
		fprintf(stderr,
			"PingPongTest: couldn't put pp to %s\n",
				r_pp_woof);
		fflushg(stderr);
		WooFDrop(l_pp_w);
		return(0);
	}

	/* wait one sec for other side to reply */
	sleep(1);

	/*
	 * retry get of local woof looking for seq_no
	 *
	 * note that other ping pong tests could be under way
	 */
	retry_count = 0;
	found = 0;
	while(retry_count < PPRETRIES) {
		/* start at the seq no we sent */
		curr_seq_no = l_pp.seq_no;
		err = WooFRead(l_pp_w,&g_pp,curr_seq_no);
		if(err < 0) {
			sleep(PPSLEEP);
			retry_count++;
			continue;
		}
		/*
		 * if we see the reply, we are good
		 */
		if(g_pp.seq_no == l_pp.seq_no) {
			found = 1;
			break;
		}
		while(g_pp.seq_no != l_pp.seq_no) {
			curr_seq_no++;
			err = WooFRead(l_pp_w,&g_pp,curr_seq_no);
			if(err < 0) {
				sleep(PPSLEEP);
				retry_count+;
				break;
			}
		}
		if(g_pp.seq_no == l_pp.seq_no) {
			found = 1;
			break;
		}
	}

	WooFDrop(l_pp_w);
	if(found == 1) {
		return(1);
	} else {
		fprintf(stderr,
			"PingPongTest: pingpong to %s failed after retries\n",
				r_pp_name);
			fflush(stderr);
		return(0);
	}

}
	

void DoClient(STATE *state, STATUS *status)
{
	return;
}

void DoMaster(STATE *state, STATUS *status)
{
	WOOF *l_state_w;
	WOOF *l_status_w;
	STATE new_state;
	STATE curr_state;
	STATUS l_status;
	STATUS r_status;
	char l_state_name[255];
	char r_state_woof[4096];
	char l_status_name[512];
	int err;
	unsigned long my_seq_no;
	unsigned long s_seq_no;
	char other_color;
	unsigned long other_seq_no;
	unsigned long last_r_seq_no;
	char client_color;

	err = WooFNameFromURI(state->wname,l_state_name,sizeof(l_state_name));
	if(err < 0) {
		fprintf(stderr,
			"DoMaster: failed to extract state name from %s\n",
			state->wname);
		return;
	}
	MAKE_EXTENDED_NAME(l_status_name,l_state_name,"status");
	
	/* open the woofs */
	l_state_w = WooFOpen(l_state_name);
	if(l_state_w == NULL) {
		fprintf(stderr,
			"DoMaster: failed to open state from %s\n",
			l_state_name);
		return;
	}
	l_status_w = WooFOpen(l_status_name);
	if(l_status_w == NULL) {
		fprintf(stderr,
			"DoMaster: failed to open status from %s\n",
			l_status_name);
		WooFDrop(l_state_w);
		return;
	}

	/* get my current state */
	my_seq_no = WooFLatestSeqno(l_state_w);
	err = WooFRead(l_state_w,&curr_state,my_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"DoMaster: failed to read state from %s\n",
			l_status_name);
		WooFDrop(l_state_w);
		WooFDrop(l_status_w);
		return;
	}

	/* now get current remote state */
	sprintf(r_state_woof,"woof://%s/%s",
		state->other_ip,
		l_state_name);
	other_seq_no = WooFGetLatestSeqno(r_state_woof);
	if(WooFInvalid(other_seq_no)) {
		fprintf(stderr,
			"DoMaster: bad seq no from %s\n",
				r_state_woof);
		fflush(stderr);
		other_color = 'R';
	} else {
		other_color = 'G';
	}

	/* read the last status the remote side wrote to us */
	last_r_seq_no = WooFLatestSeqno(l_status_w);
	err = WooFRead(l_status_w,&r_status,last_r_seqno);
	if(err < 0) {
		fprintf(stderr,"DoMaster: couldn't read latest from %s\n",
			l_status_name);
		fflush(stderr);
		WooFDrop(l_state_w);
		WooFDrop(l_status_w);
		return;
	}

	memcpy(&new_state,&curr_state,sizeof(new_state));
	/*
	 * if latest remote is green and remote seq_no is bigger, 
	 * assume I have been down and
	 * believe the other side
	 */
	if((other_color == 'G') && (other_seq_no > last_r_seq_no)) {
		new_state.my_state = r_status.local;
		new_state.other_color = other_color;
	/*
	 * sanity check -- shouldn't happen
	 */
	} else if((other_color == 'G') && (other_seq_no > r_seq_no)) { 
		fprintf(stderr,
		"DoMaster: state error, other green, osn: %lu, rsn: %lu\n",
			other_seq_no,r_seq_no);
		fflush(stderr);
		WooFDrop(l_state_w);
		WoofDrop(l_status_w);
		return;
	} else {
	/*
	 * we are up to date with respect to remote side
	 * or remote side is red
	 */
		err = PingPongTest(state,'C');
		if(err == 0) {
			client_color = 'R';
		} else if(err == 1) {
			client_color = 'G';
		} else {
			fprintf(stderr,
			"DoMaster: pp test to client failed internally\n");
			fflush(stderr);
			WooFDrop(l_state_w);
			WooFDrop(l_status_w);
			return;
		}
		if(client_color == 'G') {
			new_state.my_state = 'M';
			new_state.client_color = client_color;
XXX
		  
		

	
		

	


	return;
}

void DoSlave(STATE *state, STATUS *status)
{
	return;
}




int MSPulseHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

	PULSE *pstc = (PULSE *)ptr;
	char state_woof[4096];
	STATE l_state;  /* my state */
	STATUS l_status;
	WOOF *l_state_w;
	WOOF *l_status_w;
	char l_state_woof[255];
	char l_status_woof[512];
	unsigned long seq_no;
	int err;
	char other_color;
	char client_other;

#ifdef DEBUG
	printf("MSPulseHandler: called on %s with seq_no: %lu, time: %d\n",
		pstc.wname,
		pstc.last_seq_no,
		(double)(pstc.tm.tv_sec));
#endif

	err = WooFNameFromURI(pstc.wname, l_state_woof, sizeof(l_state_woof));

	if(err < 0) {
		fprintf(stderr,"MSPulseHandler: couldn't extract local state woof name from %s\n",
			pstc.wname);
		fflush(stderr);
		exit(1);
	}

	/*
	 * open the woof in this name space so we can read the current state
	 */
	l_state_w = WooFOpen(l_state_woof);
	if(l_state_w == NULL) {
		fprintf(stderr,"MSPulseHandler: couldn't open local woof %s\n",
			l_state_woof);
		fflush(stderr);
		exit(1);
	}

	MAKE_EXTENDED_NAME(l_status_woof,l_state_woof,sizeof(l_status_woof));

	/*
	 * open status woof so we can put new status
	 */
	l_status_w = WooFOpen(l_status_woof);
	if(l_status_w == NULL) {
		fprintf(stderr,"MSPulseHandler: couldn't open local status woof %s\n",
			l_status_woof);
		fflush(stderr);
		exit(1);
	}

	err = WooFRead(l_state_w,&l_state,sizeof(l_state));
	if(err < 0 ) {
		fprintf(stderr,
			"MSPulseHandler: couldn't get local state from %s\n",
				pstc.wname);
		fflush(stderr);
		exit(1);
	}
	err = WooFRead(l_status_w,&l_status,sizeof(l_status));
	if(err < 0 ) {
		fprintf(stderr,
			"MSPulseHandler: couldn't get local status from %s\n",
				l_status_woof);
		fflush(stderr);
		exit(1);
	}

	switch(l_state.my_state) {
		case 'C':
		case 'c':
			DoClient(&l_state,&l_status);
			break
		case 'M':
		case 'm':
			DoMaster(&l_state,&l_status);
			break;
		case 'S':
		case 's':
			DoSlave(&l_state,&l_status);
			break;
		default:
			fprintf(stderr,
				"MSPulseHandler: bad local state %s\n",
					l_state.my_state);
			exit(1);
	}

	return(1);

}
