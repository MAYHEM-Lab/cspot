#ifndef SENSPOT_H
#define SENSPOT_H

#include "hval.h"
#include <string.h>

struct senspot_stc
{
	char type;
	Hval value;
	char ip_addr[25];
	unsigned char payload[1024]; // for strings
};

#define SENSPOT_ASSIGN((s),(t),(v)) \{
	s->type = (t);\
	switch(t) {\
		case 'd':\
		case 'D':\
			s->value.d = atod((v));\
			break;\
		case 's':\
		case 'S':\
			strncpy(s->payload,v,sizeof(s->payload));\
			s->value.s = payload;\
			break;\
		case 'i':\
		case 'i':\
			s->value.i = atoi((v));\
			break;\
		case 'l':\
		case 'L':\
			s->value.l = atol((v));\
			break;\
		default:\
			fprintf(stderr,"bad type %c in SENSPOT_ASSIGN\n",(t));
			fflush(stderr);\
			s->value.v = NULL;\
	}

typedef struct senspot_stc SENSPOT;

#endif

