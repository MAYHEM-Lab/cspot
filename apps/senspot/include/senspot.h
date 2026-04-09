#ifndef SENSPOT_H
#define SENSPOT_H

#include "hval.h"
#include <string.h>
#include <time.h>
#ifdef JUMBO
#define PAYLOAD (8*1024)
#else
#define PAYLOAD (1*2024)
#endif

#define FPAYLOAD (8*1024)

struct senspot_stc
{
	char type;
	Hval value;
	char ip_addr[25];
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned int dedup_seqno;
	unsigned int send_size; // for file xfer
	unsigned char payload[PAYLOAD]; // for strings
};

typedef struct senspot_stc SENSPOT;

struct senspot_file_stc
{
	unsigned int proto;
	unsigned int flags;
	unsigned int version;
	unsigned int creation_time;
	unsigned int dedup_seqno;
	unsigned int woof_end; // seqno in woof containing end record
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned int payload_size;
	unsigned char payload[FPAYLOAD];
};

typedef struct senspot_file_stc SENSFILE;

struct senspot_file_mv_str
{
	unsigned int proto;
	unsigned int flags;
	unsigned int version;
	unsigned int creation_time;
	unsigned int dedup_seqno;
	unsigned int woof_end; // seqno in woof containing end record
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned int payload_size;
	unsigned int file_size;
};

typedef struct senspot_file_mv_str SENSMV;

#define PROTO_1 (1)
#define PROTO_2 (2)

#define SENS_START (1)
#define SENS_EOF (2)
unsigned int LastFileVersion(char *wname);

void SenspotPrint(SENSPOT *spt, unsigned long seq_no);
void SenspotAssign(SENSPOT *spt, char type, char *v);

struct senspot_forward_stc
{
	char forward_woof[4096];
	unsigned last_local;
	unsigned last_remote;
};

typedef struct senspot_forward_stc SENSFWD;

struct senspot_fwd_state_stc
{
	unsigned seq_no;
	int state;
};

typedef struct senspot_fwd_state_stc SENSFWDSTATE;
#define FWDACTIVE (1)
#define FWDIDLE (2)

#endif

