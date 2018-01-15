#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include "woofc.h"
#include "senspot.h"

/**
 * HACK
 *
 * The original code puts a struct timeval in the payload.  On 32 bit machine
 * (Rpi) this does not decode properly when get runs on 64 bit machine.  The
 * fix is to put htonl() and ntohl() in explictly, but then "old" woofs don't
 * work
 *
 * This hack let's old 64-bit woofs work with new 32/64 bit code
 **/
#define BADTIME (1516047601.0 + (100.0 * 365.0 * 86400.0))
#define NBADTIME (1516047601.0 - (365.0 * 86400.0))

void SenspotPrint(SENSPOT *spt, unsigned long seq_no)
{
	double ts;
	struct timeval tm;
	struct timeval *ptm;

	switch(spt->type) {
		case 'd':
		case 'D':
			fprintf(stdout,"%f ",spt->value.d);
			break;
		case 's':
		case 'S':
			fprintf(stdout,"%s ",spt->payload);
			break;
		case 'i':
		case 'I':
			fprintf(stdout,"%d ",spt->value.i);
			break;
		case 'l':
		case 'L':
			fprintf(stdout,"%lu ",spt->value.l);
			break;
		default:
			fprintf(stdout,"unknown_senspot_type-%c ",spt->type);
			break;
	}

	tm.tv_sec = (unsigned long)ntohl(spt->tv_sec);
	tm.tv_usec = (unsigned long)ntohl(spt->tv_usec);
	ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);
printf("ts: %f\n",ts);
	if(!((ts < (double)BADTIME) && (ts > (double)NBADTIME))) {
		ptm = (struct timeval *)&(spt->tv_usec);
		tm.tv_sec = ptm->tv_sec;
		tm.tv_usec = ptm->tv_usec;
		ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);
printf("bad: %f, ts: %f\n",(double)BADTIME,ts);
	}

	fprintf(stdout,"time: %10.10f ",ts);
	fprintf(stdout,"%s ",spt->ip_addr);
	fprintf(stdout,"seq_no: %lu\n",seq_no);
	fflush(stdout);

	return;
}

void SenspotAssign(SENSPOT *spt, char type, char *v)
{
	
	spt->type = type;
	switch(type) {
		case 'd':
		case 'D':
//			spt->value.d = atof(v);
			spt->value.d = strtod((char *)v,NULL);
			break;
		case 's':
		case 'S':
			strncpy(spt->payload,v,sizeof(spt->payload));
			spt->value.s = v;
			break;
		case 'i':
		case 'I':
			spt->value.i = atoi(v);
			break;
		case 'l':
		case 'L':
			spt->value.l = atol(v);
			break;
		default:
			fprintf(stderr,"bad type %c in SenspotAssign\n",type);
			fflush(stderr);
			spt->value.v = NULL;
	}

	return;
}

