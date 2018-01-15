#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include "woofc.h"
#include "senspot.h"

void SenspotPrint(SENSPOT *spt, unsigned long seq_no)
{
	double ts;
	struct timeval tm;

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

	tm.tv_sec = ntohl(spt->tm.tv_sec);
	tm.tv_usec = ntohl(spt->tm.tv_usec);
	ts = (double)((unsigned long)tm.tv_sec) + ((double)((unsigned long)tm.tv_usec) / 1000000.0);

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

