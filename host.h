#ifndef HOST_H
#define HOST_H

#include <pthread.h>
#include "mio.h"

struct host_stc
{
	unsigned long host_id;
	unsigned long long max_seen;
	unsigned long long event_horizon;
};

typedef struct host_stc HOST;

struct host_list_stc
{
	pthread_mutex_t lock;
	char filename[4096]; /* can't be a point for MIO persistence */
	HOST *hash;
	MIO *h_mio;
	unsigned int hash_size;
	unsigned long count;
};

typedef struct host_list_stc HOSTLIST;

#define HASHCOUNT 100
#define HOSTSIZE (HASHCOUNT * sizeof(HOST) + sizeof(HOSTLIST))

HOSTLIST *HostListCreate();
void HostListFree(HOSTLIST *hl);
int HostListAdd(HOSTLIST *hl, unsigned long host_id);
HOST *HostListFind(HOSTLIST *hl, unsigned long host_id);


#endif

