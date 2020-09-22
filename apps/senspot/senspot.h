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
	unsigned int pad_1;
	unsigned int pad_2;
#ifdef JUMBO
	unsigned char payload[8*1024]; // for strings
#else
	unsigned char payload[1024]; // for strings
#endif
};

typedef struct senspot_stc SENSPOT;

void SenspotPrint(SENSPOT *spt, unsigned long seq_no);
void SenspotAssign(SENSPOT *spt, char type, char *v);

struct senspot_forward_stc
{
	char forward_woof[4096];
	unsigned last_local;
	unsigned last_remote;
};

typedef struct senspot_forward_stc SENSFWD;

#endif

