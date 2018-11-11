#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-access.h"
#include "master-slave.h"

#define DEBUG

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

	err = WooFNameFromURI(state->wname,l_name,sizeof(l_name));
	if(err < 0) {
		fprintf(stderr,
			"PingPongTest: couldn't extract local name from %s\n",
				state->wname);
		fflush(stderr);
		return(-1);
	}
	memset(l_pp.return_woof,0,sizeof(l_pp.return_woof));
	memset(l_pp_name,0,sizeof(l_pp_name));
	MAKE_EXTENDED_NAME(l_pp.return_woof,state->wname,"pingpong");
	MAKE_EXTENDED_NAME(l_pp_name,l_name,"pingpong");

	/* make remore pingpong woof name */
	memset(r_pp_woof,0,sizeof(r_pp_woof));
	if(target == 'O') {
		sprintf(r_pp_woof,"woof://%s/%s",state->other_ip,l_pp_name);
	} else if(target == 'C') {
		sprintf(r_pp_woof,"woof://%s/%s",state->client_ip,l_pp_name);
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
		fflush(stderr);
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
		fflush(stderr);
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
				retry_count++;
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
				r_pp_woof);
			fflush(stderr);
		return(0);
	}

}

void PrintState(STATE *state)
{
	printf("%s\n",state->wname);
	printf("\tip: %s\n",state->my_ip);
	printf("\t\tstate: %c\n",state->my_state);
	printf("\tother_ip: %s\n",state->other_ip);
	printf("\t\tother_state: %c\n",state->other_state);
	printf("\t\tother_color: %c\n",state->other_color);
	printf("\tclient_ip: %s\n",state->client_ip);
	printf("\t\tclient_color: %c\n",state->client_color);
	fflush(stdout);
	return;
}

void PrintStatus(char *wname, STATUS *status)
{
	printf("%s\n",wname);
	printf("\tlocal_state: %c\n",status->local);
	printf("\tremote_state: %c\n",status->remote);
	printf("\tremote_seq_no: %c\n",status->remote_seq_no);
	fflush(stdout);
	return;
}
	
void DoClient(STATE *state)
{
	char m1_woof[4096];
	char m1_status_woof[4096];
	char m2_woof[4096];
	char m2_status_woof[4096];
	STATE m1_state;
	STATE m2_state;
	STATUS m1_status;
	STATUS m2_status;
	int err;
	unsigned long m1_seq_no;
	unsigned long m2_seq_no;
	unsigned long m1_status_seq_no;
	unsigned long m2_status_seq_no;
	char m1_state_ok;
	char m1_status_ok;
	char m2_state_ok;
	char m2_status_ok;
	char wname[256];

	err = WooFNameFromURI(state->wname,wname,sizeof(wname));
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoClient: couldn't get woof name from %s\n",
				state->wname);
		fflush(stderr);
		return;
	}
	memset(m1_woof,0,sizeof(m1_woof));
	memset(m2_woof,0,sizeof(m2_woof));
	/* original master in my_ip */
	sprintf(m1_woof,"woof://%s/%s",state->my_ip,wname);
	MAKE_EXTENDED_NAME(m1_status_woof,m1_woof,"status");
	sprintf(m2_woof,"woof://%s/%s",state->other_ip,wname);
	MAKE_EXTENDED_NAME(m2_status_woof,m2_woof,"status");

	m1_state_ok = 1;
	m1_status_ok = 1;
	m1_seq_no = WooFGetLatestSeqno(m1_woof);
	if(WooFInvalid(m1_seq_no)) {
		fprintf(stderr,
			"ERROR DoClient: bad seq no from m1 %s\n",
				m1_woof);
		fflush(stderr);
		m1_state_ok = 0;
	}
	err = WooFGet(m1_woof,&m1_state,m1_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoClient: couldn't fetch state from %s\n",
				m1_woof);
		fflush(stderr);
		m1_state_ok = 0;
	}
	m1_status_seq_no = WooFGetLatestSeqno(m1_status_woof);
	if(WooFInvalid(m1_status_seq_no)) {
		fprintf(stderr,
		"ERROR DoClient: couldn't fetch status seq_no from %s\n",
				m1_status_woof);
		fflush(stderr);
		m1_status_ok = 0;
	}
	err = WooFGet(m1_status_woof,&m1_status,m1_status_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoClient: couldn't fetch status from %s\n",
				m1_status_woof);
		fflush(stderr);
		m1_status_ok = 0;
	} 

	if(m1_state_ok == 1) {
		PrintState(&m1_state);
	} else {
		printf("DoClient: no state for %s\n",m1_woof);
		fflush(stdout);
	}
	if(m1_status_ok == 1) {
		PrintStatus(m1_status_woof,&m1_status);
	} else {
		printf("DoClient: no status for %s\n",m1_status_woof);
		fflush(stdout);
	}

	m2_state_ok = 1;
	m2_status_ok = 1;
	m2_seq_no = WooFGetLatestSeqno(m2_woof);
	if(WooFInvalid(m2_seq_no)) {
		fprintf(stderr,
			"ERROR DoClient: bad seq no from m2 %s\n",
				m2_woof);
		fflush(stderr);
		m2_state_ok = 0;
	}
	err = WooFGet(m2_woof,&m2_state,m2_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoClient: couldn't fetch state from %s\n",
				m2_woof);
		fflush(stderr);
		m2_state_ok = 0;
	}
	m2_status_seq_no = WooFGetLatestSeqno(m2_status_woof);
	if(WooFInvalid(m2_status_seq_no)) {
		fprintf(stderr,
		"ERROR DoClient: couldn't fetch status seq_no from %s\n",
				m2_status_woof);
		fflush(stderr);
		m2_status_ok = 0;
	}
	err = WooFGet(m2_status_woof,&m2_status,m2_status_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoClient: couldn't fetch status from %s\n",
				m2_status_woof);
		fflush(stderr);
		m2_status_ok = 0;
	} 

	if(m2_state_ok == 1) {
		PrintState(&m2_state);
	} else {
		printf("DoClient: no state for %s\n",m2_woof);
		fflush(stdout);
	}
	if(m2_status_ok == 1) {
		PrintStatus(m2_status_woof,&m2_status);
	} else {
		printf("DoClient: no status for %s\n",m2_status_woof);
		fflush(stdout);
	}


	return;
}

void DoMaster(STATE *state)
{
	WOOF *l_state_w;
	WOOF *l_status_w;
	STATE new_state;
	STATE r_state;
	STATUS l_status;
	STATUS r_status;
	char l_state_name[255];
	char r_state_woof[4096];
	char r_status_woof[4096];
	char l_status_name[512];
	unsigned long r_seq_no;
	unsigned long other_seq_no;
	unsigned long last_r_seq_no;
	unsigned long new_seq_no;
	char other_color;
	char client_color;
	int err;

	err = WooFNameFromURI(state->wname,l_state_name,sizeof(l_state_name));
	if(err < 0) {
		fprintf(stderr,
		"ERROR DoMaster: failed to extract state name from %s\n",
			state->wname);
		return;
	}
	MAKE_EXTENDED_NAME(l_status_name,l_state_name,"status");
	
	/* open the woofs */
	l_state_w = WooFOpen(l_state_name);
	if(l_state_w == NULL) {
		fprintf(stderr,
			"ERROR DoMaster: failed to open state from %s\n",
			l_state_name);
		return;
	}
	l_status_w = WooFOpen(l_status_name);
	if(l_status_w == NULL) {
		fprintf(stderr,
			"ERROR DoMaster: failed to open status from %s\n",
			l_status_name);
		WooFDrop(l_state_w);
		return;
	}

	/* now get current remote state */
	memset(r_state_woof,0,sizeof(r_state_woof));
	sprintf(r_state_woof,"woof://%s/%s",
		state->other_ip,
		l_state_name);
	other_seq_no = WooFGetLatestSeqno(r_state_woof);
	if(WooFInvalid(other_seq_no)) {
		fprintf(stderr,
			"ERROR DoMaster: bad seq no from %s\n",
				r_state_woof);
		fflush(stderr);
		other_color = 'R';
	} else {
		other_color = 'G';
	}

	/* read the last status the remote side wrote to us */
	last_r_seq_no = WooFLatestSeqno(l_status_w);
	err = WooFRead(l_status_w,&r_status,last_r_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoMaster: couldn't read latest from %s\n",
			l_status_name);
		fflush(stderr);
		WooFDrop(l_state_w);
		WooFDrop(l_status_w);
		return;
	}

	memcpy(&new_state,state,sizeof(new_state));
	/*
	 * if latest remote is green and remote seq_no is bigger, 
	 * assume I have been down and
	 * believe the other side
	 */
	if((other_color == 'G') && (other_seq_no > r_status.remote_seq_no)) {
#ifdef DEBUG
		fprintf(stdout,
"DoMaster: other side is green and I'm out of date: me: %lu other: %lu\n",
			r_status.remote_seq_no,
			other_seq_no);
		fflush(stdout);
#endif
		r_seq_no = WooFGet(r_state_woof,&r_state,other_seq_no);
		/* if it is really red, believe last */
		if(WooFInvalid(r_seq_no)) {
			fprintf(stderr,
"ERROR DoMaster: other side goes red on state fetch, setting my state to %c\n",
				r_status.local);
			fflush(stdout);
			new_state.my_state = r_status.local;
			new_state.other_color = 'R';
			other_color = 'R';
		} else {
			new_state.other_color = other_color;
			if(r_state.my_state == 'M') {
#ifdef DEBUG
				fprintf(stdout,
"DoMaster: I'm out of date and other side is master, I'm going slave\n");
				fflush(stdout);
#endif
				new_state.my_state = 'S';
			} else if(r_state.my_state == 'S') {
#ifdef DEBUG
				fprintf(stdout,
"DoMaster: I'm out of date and other side is slave, I'm going master\n");
				fflush(stdout);
#endif
				new_state.my_state = 'M';
			} else {
				fprintf(stderr,
			"ERROR DoMaster: bad remote state out of date %s\n",
					r_state.my_state);
				fflush(stderr);
				new_state.other_color = 'R';
				other_color = 'R';
			}
		}
	/*
	 * sanity check -- shouldn't happen
	 */
	} else if((other_color == 'G') && 
			(other_seq_no < r_status.remote_seq_no)) { 
		fprintf(stderr,
	"ERROR DoMaster: state error, other green, osn: %lu, rsn: %lu\n",
			other_seq_no,r_status.remote_seq_no);
		fflush(stderr);
		WooFDrop(l_state_w);
		WooFDrop(l_status_w);
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
		"ERROR DoMaster: pp test to client failed internally\n");
			fflush(stderr);
			WooFDrop(l_state_w);
			WooFDrop(l_status_w);
			return;
		}

#ifdef DEBUG
		fprintf(stdout,
"DoMaster: client is %s\n",client_color);
		fflush(stdout);
#endif
		new_state.other_color = other_color;
		new_state.client_color = client_color;
		if(client_color == 'G') {
			/* get current state of other side */
			r_seq_no = WooFGet(r_state_woof,&r_state,other_seq_no);
			if(WooFInvalid(r_seq_no)) {
				/* other side is down */
				new_state.my_state = 'M';
				new_state.other_color = 'R';
				other_color = 'R';
#ifdef DEBUG
				fprintf(stdout,
"DoMaster: up to date, other side goes red on state fetch\n");
				fflush(stdout);
#endif
			} else if(r_state.my_state == 'S') {
				/* other side is up and a slave */
				new_state.my_state = 'M';
#ifdef DEBUG
				fprintf(stdout,
"DoMaster: up to date, other side is green and slave, I'm going master\n");
				fflush(stdout);
#endif
			} else if((r_state.my_state == 'M') &&
			  (strcmp(state->my_ip,r_state.my_ip) < 0)) {
				/* we are both masters, break tie with ip */
				new_state.my_state = 'M';
#ifdef DEBUG
				fprintf(stdout,
"DoMaster: up to date, other is green and master, I break tie, going master\n");
				fflush(stdout);
#endif
			} else if((r_state.my_state != 'M') &&
				  (r_state.my_state != 'S')) {
					fprintf(stderr,
				"ERROR DoMaster: bad remote state %c\n",
					r_state.my_state);
					fflush(stderr);
				new_state.my_state = 'M';
				new_state.other_color = 'R';
				other_color = 'R';
			} else {
#ifdef DEBUG
				fprintf(stdout,
"DoMaster: up to date, other side is green and master and smaller, going slave\n");
				fflush(stdout);
#endif
				new_state.my_state = 'S';
			}
		} else /* client is red */ {
#ifdef DEBUG
			fprintf(stdout,
"DoMaster: client is red, goiung slave\n");
			fflush(stdout);
#endif
			new_state.my_state = 'S';
		}
	}

	/* put my new state */
	new_seq_no = WooFAppend(l_state_w,NULL,&new_state);

	/* update status on other side */
	if(new_state.my_state == 'M') {
		l_status.remote = 'M'; /* I am remote to other side */
		l_status.local = 'S';
	} else {
		l_status.remote = 'S'; /* I am remote to other side */
		l_status.local = 'M';
	}
	l_status.remote_seq_no = new_seq_no;

	/* make remote woof status name */
	memset(r_status_woof,0,sizeof(r_status_woof));
	sprintf(r_status_woof,"woof://%s/%s",
		new_state.other_ip,
		l_status_name);

	/* update the other side */
	r_seq_no = WooFPut(r_status_woof,NULL,&l_status);
	if(WooFInvalid(r_seq_no)) {
		fprintf(stderr,
		  "ERROR DoMaster: bad status put\n");
		new_state.other_color = 'R';
		new_seq_no = WooFAppend(l_state_w,NULL,&new_state);
	}

			
	WooFDrop(l_state_w);
	WooFDrop(l_status_w);

	return;
}

void DoSlave(STATE *state)
{
	WOOF *l_state_w;
	WOOF *l_status_w;
	STATE new_state;
	STATE r_state;
	STATUS l_status;
	STATUS r_status;
	char l_state_name[255];
	char r_state_woof[4096];
	char r_status_woof[4096];
	char l_status_name[512];
	unsigned long r_seq_no;
	unsigned long other_seq_no;
	unsigned long last_r_seq_no;
	unsigned long new_seq_no;
	char other_color;
	char client_color;
	int err;

	err = WooFNameFromURI(state->wname,l_state_name,sizeof(l_state_name));
	if(err < 0) {
		fprintf(stderr,
		"ERROR DoSlave: failed to extract state name from %s\n",
			state->wname);
		return;
	}
	MAKE_EXTENDED_NAME(l_status_name,l_state_name,"status");
	
	/* open the woofs */
	l_state_w = WooFOpen(l_state_name);
	if(l_state_w == NULL) {
		fprintf(stderr,
			"ERROR DoSlave: failed to open state from %s\n",
			l_state_name);
		return;
	}
	l_status_w = WooFOpen(l_status_name);
	if(l_status_w == NULL) {
		fprintf(stderr,
			"ERROR DoSlave: failed to open status from %s\n",
			l_status_name);
		WooFDrop(l_state_w);
		return;
	}

	/* now get current remote state */
	memset(r_state_woof,0,sizeof(r_state_woof));
	sprintf(r_state_woof,"woof://%s/%s",
		state->other_ip,
		l_state_name);
	other_seq_no = WooFGetLatestSeqno(r_state_woof);
	if(WooFInvalid(other_seq_no)) {
		fprintf(stderr,
			"ERROR DoSlave: bad seq no from %s\n",
				r_state_woof);
		fflush(stderr);
		other_color = 'R';
	} else {
		other_color = 'G';
	}

	/* read the last status the remote side wrote to us */
	last_r_seq_no = WooFLatestSeqno(l_status_w);
	err = WooFRead(l_status_w,&r_status,last_r_seq_no);
	if(err < 0) {
		fprintf(stderr,
			"ERROR DoSlave: couldn't read latest from %s\n",
			l_status_name);
		fflush(stderr);
		WooFDrop(l_state_w);
		WooFDrop(l_status_w);
		return;
	}

	memcpy(&new_state,state,sizeof(new_state));
	/*
	 * if latest remote is green and remote seq_no is bigger, 
	 * assume I have been down and
	 * believe the other side
	 */
	if((other_color == 'G') && (other_seq_no > r_status.remote_seq_no)) {
#ifdef DEBUG
		fprintf(stdout,
"DoSlave: other side is green and I'm out of date: me: %lu other: %lu\n",
			r_status.remote_seq_no,
			other_seq_no);
		fflush(stdout);
#endif
		r_seq_no = WooFGet(r_state_woof,&r_state,other_seq_no);
		/* if it is really red, believe last */
		if(WooFInvalid(r_seq_no)) {
			fprintf(stderr,
"ERROR DoSlave: other side goes red on state fetch, setting my state to %c\n",
				r_status.local);
			fflush(stdout);
			new_state.my_state = r_status.local;
			new_state.other_color = 'R';
			other_color = 'R';
		} else {
			new_state.other_color = other_color;
			if(r_state.my_state == 'M') {
#ifdef DEBUG
				fprintf(stdout,
"DoSlave: I'm out of date and other side is master, I'm going slave\n");
				fflush(stdout);
#endif
				new_state.my_state = 'S';
			} else if(r_state.my_state == 'S') {
#ifdef DEBUG
				fprintf(stdout,
"DoSlave: I'm out of date and other side is slave, I'm going master\n");
				fflush(stdout);
#endif
				new_state.my_state = 'M';
			} else {
				fprintf(stderr,
			"ERROR DoSlave: bad remote state out of date %s\n",
					r_state.my_state);
				fflush(stderr);
				new_state.other_color = 'R';
				other_color = 'R';
			}
		}
	/*
	 * sanity check -- shouldn't happen
	 */
	} else if((other_color == 'G') && 
			(other_seq_no < r_status.remote_seq_no)) { 
		fprintf(stderr,
	"ERROR DoSlave: state error, other green, osn: %lu, rsn: %lu\n",
			other_seq_no,r_status.remote_seq_no);
		fflush(stderr);
		WooFDrop(l_state_w);
		WooFDrop(l_status_w);
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
		"ERROR DoSlave: pp test to client failed internally\n");
			fflush(stderr);
			WooFDrop(l_state_w);
			WooFDrop(l_status_w);
			return;
		}

#ifdef DEBUG
		fprintf(stdout,
"DoSlave: client is %s\n",client_color);
		fflush(stdout);
#endif
		new_state.other_color = other_color;
		new_state.client_color = client_color;
		if(client_color == 'G') {
			/* get current state of other side */
			r_seq_no = WooFGet(r_state_woof,&r_state,other_seq_no);
			if(WooFInvalid(r_seq_no)) {
				/* other side is down */
				new_state.my_state = 'M';
				new_state.other_color = 'R';
				other_color = 'R';
#ifdef DEBUG
				fprintf(stdout,
"DoSlave: up to date, other side goes red on state fetch, going master\n");
				fflush(stdout);
#endif
			} else if(r_state.my_state == 'M') {
				/* other side is up and a master */
				new_state.my_state = 'S';
#ifdef DEBUG
				fprintf(stdout,
"DoSlave: up to date, other side is green and master, I'm going slave\n");
				fflush(stdout);
#endif
			} else if((r_state.my_state == 'S') &&
			  (strcmp(state->my_ip,r_state.my_ip) < 0)) {
				/* we are both slaves, break tie with ip */
				new_state.my_state = 'M';
#ifdef DEBUG
				fprintf(stdout,
"DoSlave: up to date, other is green and slave, I break tie, going master\n");
				fflush(stdout);
#endif
			} else if((r_state.my_state != 'M') &&
				  (r_state.my_state != 'S')) {
					fprintf(stderr,
				"ERROR DoSlave: bad remote state %c\n",
					r_state.my_state);
					fflush(stderr);
				new_state.my_state = 'M';
				new_state.other_color = 'R';
				other_color = 'R';
			} else {
#ifdef DEBUG
				fprintf(stdout,
"DoSlave: up to date, other side is green and slave and smaller, going slave\n");
				fflush(stdout);
#endif
				new_state.my_state = 'S';
			}
		} else /* client is red */ {
#ifdef DEBUG
			fprintf(stdout,
"DoSlave: client is red, going slave\n");
			fflush(stdout);
#endif
			new_state.my_state = 'S';
		}
	}

	/* put my new state */
	new_seq_no = WooFAppend(l_state_w,NULL,&new_state);

	/* update status on other side */
	if(new_state.my_state == 'M') {
		l_status.remote = 'M'; /* I am remote to other side */
		l_status.local = 'S';
	} else {
		l_status.remote = 'S'; /* I am remote to other side */
		l_status.local = 'M';
	}
	l_status.remote_seq_no = new_seq_no;

	/* make remote woof status name */
	memset(r_status_woof,0,sizeof(r_status_woof));
	sprintf(r_status_woof,"woof://%s/%s",
		new_state.other_ip,
		l_status_name);

	/* update the other side */
	r_seq_no = WooFPut(r_status_woof,NULL,&l_status);
	if(WooFInvalid(r_seq_no)) {
		fprintf(stderr,
		  "ERROR DoSlave: bad status put\n");
		new_state.other_color = 'R';
		new_seq_no = WooFAppend(l_state_w,NULL,&new_state);
	}
			
	WooFDrop(l_state_w);
	WooFDrop(l_status_w);

	return;
}
		
int MSPulseHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

	PULSE *pstc = (PULSE *)ptr;
	STATE l_state;  /* my state */
	WOOF *l_state_w;
	char l_state_woof[255];
	unsigned long my_seq_no;
	int err;

#ifdef DEBUG
	printf("MSPulseHandler: called on %s with seq_no: %lu, time: %d\n",
		pstc->wname,
		pstc->last_seq_no,
		(double)(pstc->tm.tv_sec));
#endif

	err = WooFNameFromURI(pstc->wname, l_state_woof, sizeof(l_state_woof));

	if(err < 0) {
		fprintf(stderr,"MSPulseHandler: couldn't extract local state woof name from %s\n",
			pstc->wname);
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


	my_seq_no = WooFLatestSeqno(l_state_w);
	err = WooFRead(l_state_w,&l_state,my_seq_no);
	if(err < 0 ) {
		fprintf(stderr,
			"MSPulseHandler: couldn't get local state from %s\n",
				pstc->wname);
		fflush(stderr);
		exit(1);
	}
	switch(l_state.my_state) {
		case 'C':
		case 'c':
			DoClient(&l_state);
			break;
		case 'M':
		case 'm':
			DoMaster(&l_state);
			break;
		case 'S':
		case 's':
			DoSlave(&l_state);
			break;
		default:
			fprintf(stderr,
				"MSPulseHandler: bad local state %s\n",
					l_state.my_state);
			exit(1);
	}

	return(1);

}
