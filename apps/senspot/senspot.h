#ifndef SENSPOT_H
#define SENSPOT_H

#include "hval.h"
#include <string.h>
#include <time.h>

struct senspot_stc
{
	char type;
	Hval value;
	char ip_addr[25];
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned char payload[1024]; // for strings
};

typedef struct senspot_stc SENSPOT;

void SenspotPrint(SENSPOT *spt, unsigned long seq_no);
void SenspotAssign(SENSPOT *spt, char type, char *v);

#endif

