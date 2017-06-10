#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "host.h"
#include "redblack.h"

HOSTLIST *HostListCreate(char *filename)
{
	HOSTLIST *hl;
	MIO *mio;

	mio = MIOOpen(filename,"w+",HOSTSIZE);
	if(mio == NULL) {
		return(NULL);
	}

	/*
	 * lay the host list in at the top of the MIO buffer
	 */
	hl = (HOSTLIST *)MIOAddr(mio);
	hl->h_mio = mio;
	memset(hl->filename,0,sizeof(hl->filename));
	strcpy(hl->filename,filename);

	hl->count = 0;
	hl->hash_size = HASHCOUNT;
	hl->hash = (HOST *)(MIOAddr(mio) + sizeof(HOSTLIST));
	memset(hl->hash,0,HASHCOUNT*sizeof(HOST));

	pthread_mutex_init(&hl->lock,NULL);

	return(hl);
}

void HostListFree(HOSTLIST *hl) 
{
	if(hl == NULL) {
		return;
	}

	MIOClose(hl->h_mio);

	return;
}

int HostListAdd(HOSTLIST *hl, unsigned long host_id)
{
	unsigned long hash_id;
	HOST *host_rec;

	if(hl == NULL) {
		fprintf(stderr,"HostListAdd: NULL for %lu\n",host_id);
		fflush(stderr);
		return(-1);
	}

	if((hl->count + 1) >= hl->hash_size) {
		fprintf(stderr,"HostListAdd: count: %lu hash_size: %d\n",
			hl->count+1,
			hl->hash_size);
		fflush(stderr);
		return(-1);
	}

	hash_id = host_id % hl->hash_size; /* need better hash function */

	pthread_mutex_lock(&hl->lock);
	/*
	 * find empty slot
	 */
	host_rec = (HOST *)&hl->hash[hash_id];
	while(host_rec->host_id != 0) {
		hash_id = (hash_id + 1) % hl->hash_size;
		host_rec = (HOST *)&hl->hash[hash_id];
	}

	host_rec->host_id = host_id;
	hl->count += 1;
	
	pthread_mutex_unlock(&hl->lock);

	return(1);
}


HOST *HostListFind(HOSTLIST *hl, unsigned long host_id)
{
	HOST *host_rec;
	unsigned long hash_id;
	unsigned long start_id;

	if(hl == NULL) {
		return(NULL);
	}

	hash_id = host_id % hl->hash_size;

	start_id = hash_id;

	pthread_mutex_lock(&hl->lock);

	host_rec = (HOST *)&hl->hash[hash_id];
	while(host_rec->host_id != host_id) {
		if(host_rec->host_id == 0) {
			pthread_mutex_unlock(&hl->lock);
			return(NULL);
		}
		hash_id = (hash_id + 1) % hl->hash_size;
		if(hash_id == start_id) {
			pthread_mutex_unlock(&hl->lock);
			return(NULL);
		}
		host_rec = (HOST *)&hl->hash[hash_id];
	}

	pthread_mutex_unlock(&hl->lock);

	return(host_rec);
}
	
