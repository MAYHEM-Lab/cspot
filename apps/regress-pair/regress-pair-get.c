#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "regress-pair.h"

#define ARGS "W:LS:s:"
char *Usage = "regress-pair-get -W woof_name for get\n\
\t-L use same namespace for source and target\n\
\t-s series_type ('r' result, 'm' measurement, 'p' predicted, 'e' error, 'a' all)\n\
\t-S seq_no <sequence number to get, latest if missing)\n";

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

void RegressPairPrint(REGRESSVAL *rv, unsigned long seq_no)
{
	double ts;
	struct timeval tm;
	struct timeval *ptm;

	fprintf(stdout,"%f ",rv->value.d);

	tm.tv_sec = (unsigned long)ntohl(rv->tv_sec);
	tm.tv_usec = (unsigned long)ntohl(rv->tv_usec);
	ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);
	fprintf(stdout,"time: %10.10f ",ts);
	fprintf(stdout,"%s ",rv->ip_addr);
	fprintf(stdout,"seq_no: %lu\n",seq_no);
	fflush(stdout);

	return;
}

void RegressPairPrintAll(REGRESSVAL *r_rv, unsigned long r_seq_no,
			 REGRESSVAL *p_rv, unsigned long p_seq_no,
			 REGRESSVAL *m_rv, unsigned long m_seq_no)
{
	double r_ts;
	double p_ts;
	double m_ts;
	struct timeval tm;

	tm.tv_sec = (unsigned long)ntohl(r_rv->tv_sec);
	tm.tv_usec = (unsigned long)ntohl(r_rv->tv_usec);
	r_ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);

	tm.tv_sec = (unsigned long)ntohl(p_rv->tv_sec);
	tm.tv_usec = (unsigned long)ntohl(p_rv->tv_usec);
	p_ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);

	tm.tv_sec = (unsigned long)ntohl(m_rv->tv_sec);
	tm.tv_usec = (unsigned long)ntohl(m_rv->tv_usec);
	m_ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);

	fprintf(stdout,"result: %f %10.10f %lu | ",
			r_rv->value.d,r_ts,r_seq_no);
	fprintf(stdout,"pred: %f %10.10f %lu | ",
			p_rv->value.d,p_ts,p_seq_no);
	fprintf(stdout,"meas: %f %10.10f %lu\n",
			m_rv->value.d,m_ts,m_seq_no);
	fflush(stdout);

}

int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	int uselocal;
	unsigned char input_buf[4096];
	char *str;
	REGRESSVAL rv;
	REGRESSVAL r_rv;
	REGRESSVAL m_rv;
	REGRESSVAL p_rv;
	char wname[4096];
	char sname[4096+64];
	unsigned long seq_no;
	unsigned long r_seq_no;
	unsigned long m_seq_no;
	unsigned long p_seq_no;
	char series_type;

	memset(wname,0,sizeof(wname));
	seq_no = 0;
	uselocal = 0;
	series_type = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'L':
				uselocal = 1;
				break;
			case 'S':
				seq_no = atol(optarg);
				break;
			case 's':
				series_type = optarg[0];
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

	if((series_type != 'm') &&
	   (series_type != 'p') &&
	   (series_type != 'r') &&
	   (series_type != 'a') &&
	   (series_type != 'e')) {
		fprintf(stderr,
		"series type must be m, p, or r\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(series_type == 'm') { 
		MAKE_EXTENDED_NAME(sname,wname,"measured");
	} else if(series_type == 'p') {
		MAKE_EXTENDED_NAME(sname,wname,"predicted");
	} else if(series_type == 'e') {
		MAKE_EXTENDED_NAME(sname,wname,"errors");
	} else {
		MAKE_EXTENDED_NAME(sname,wname,"result");
	}

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	if(uselocal == 1) {
		WooFInit();
	}


	if((seq_no == 0) || (series_type == 'a')) {
		seq_no = WooFGetLatestSeqno(sname);
	}


	err = WooFGet(sname,&rv,seq_no);

	if(err < 0) {
		fprintf(stderr,"regress-pair-get failed for %s\n",
			sname);
		fflush(stderr);
		exit(1);
	}

	/*
	 * 'a' always gets the latest
	 */
	if(series_type == 'a') { /* results is default */
		memcpy(&r_rv,&rv,sizeof(r_rv));
		r_seq_no = seq_no;
		MAKE_EXTENDED_NAME(sname,wname,"measured");
		m_seq_no = WooFGetLatestSeqno(sname);
		err = WooFGet(sname,&m_rv,m_seq_no);
		if(err < 0) {
			fprintf(stderr,"regress-pair-get failed for %s\n",
			sname);
			fflush(stderr);
			exit(1);
		}
		MAKE_EXTENDED_NAME(sname,wname,"predicted");
		p_seq_no = WooFGetLatestSeqno(sname);
		err = WooFGet(sname,&p_rv,p_seq_no);
		if(err < 0) {
			fprintf(stderr,"regress-pair-get failed for %s\n",
			sname);
			fflush(stderr);
			exit(1);
		}
		RegressPairPrintAll(&r_rv,r_seq_no,
				    &p_rv,p_seq_no,
				    &m_rv,m_seq_no);
	} else {
		RegressPairPrint(&rv,seq_no);
	} 


	exit(0);
}

	
