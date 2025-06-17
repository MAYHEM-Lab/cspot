#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "redblack.h"

int CMQ_sd_cache_initd = 0;
pthread_mutex_t CMQ_sd_cache_lock;
RB *CMQ_sd_cache_idle; // indexed by hash
RB *CMQ_sd_cache_active; // indexed by sd

struct cmq_sd_cache_stc
{
	char ip_str[20];
	unsigned short port;
	int sd;
};
typedef struct cmq_sd_cache_stc CMQSD;

// from ChatGPT June 2, 2025
// FNV-1a 64-bit constants
#define FNV_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV_PRIME        0x100000001b3ULL

double cmq_sd_cache_hash(char *ip_str, unsigned short port) 
{
	const unsigned char *data;
	uint64_t hash;
	struct in_addr ip_addr;

	hash = FNV_OFFSET_BASIS;
	data = (unsigned char *)&ip_addr;

	if(ip_str != NULL) {
		if (inet_pton(AF_INET, ip_str, &ip_addr) != 1) {
		// Invalid IP address
			return -1;
		}
	} else {
		memset((void *)data,-1,sizeof(ip_addr));
	}

		// Hash the 4 bytes of the IP address
	for (int i = 0; i < 4; i++) {
		hash ^= data[i];
		hash *= FNV_PRIME;
	}

	// Hash the 2 bytes of the port number (big-endian to match network order)
	unsigned char port_bytes[2];
	port_bytes[0] = (port >> 8) & 0xFF;
	port_bytes[1] = port & 0xFF;

	for (int i = 0; i < 2; i++) {
		hash ^= port_bytes[i];
		hash *= FNV_PRIME;
	}

	return((double)hash);
}

void cmq_sd_cache_init()
{
	if(CMQ_sd_cache_initd == 1) {
		return;
	}
	pthread_mutex_init(&CMQ_sd_cache_lock,NULL);

	CMQ_sd_cache_idle = RBInitD();
	if(CMQ_sd_cache_idle == NULL) {
		return;
	}
	CMQ_sd_cache_active = RBInitI();
	if(CMQ_sd_cache_active == NULL) {
		return;
	}
	CMQ_sd_cache_initd = 1;

	return;
}


// find on idle list and move to active list
int cmq_sd_cache_find(char *ip_str, unsigned short port)
{
	CMQSD *csd = NULL;
	int sd;
	double hash;
	RB *rb;
	cmq_sd_cache_init();

	hash = cmq_sd_cache_hash(ip_str,port);
	pthread_mutex_lock(&CMQ_sd_cache_lock);
	rb = RBFindD(CMQ_sd_cache_idle,hash);
	if(rb != NULL) {
		// move to the active list
		csd = (CMQSD *)rb->value.v;
		RBDeleteD(CMQ_sd_cache_idle,rb);
		sd = csd->sd;
		RBInsertI(CMQ_sd_cache_active,sd,(Hval)((void *)csd));
	} else {
		sd = -1;
	}
	pthread_mutex_unlock(&CMQ_sd_cache_lock);
	return(sd);
}

// inserts on the active list
int cmq_sd_cache_insert(char *ip_str, unsigned short port, int sd)
{
	CMQSD *csd;
	RB *rb;

	cmq_sd_cache_init();

	csd = (CMQSD *)malloc(sizeof(CMQSD));
	if(csd == NULL) {
		return(-1);
	}
	
	pthread_mutex_lock(&CMQ_sd_cache_lock);
	// check to make sure it is not there
	rb = RBFindI(CMQ_sd_cache_active,sd);
	if(rb != NULL) { // already in the cache
		pthread_mutex_unlock(&CMQ_sd_cache_lock);
		free(csd);
		return(-1);
	}
	strncpy(csd->ip_str,ip_str,sizeof(csd->ip_str));
	csd->port = port;
	csd->sd = sd;
//printf("cache insert %d\n",sd);
//fflush(stdout);
	RBInsertI(CMQ_sd_cache_active,sd,(Hval)((void *)csd));
	pthread_mutex_unlock(&CMQ_sd_cache_lock);
	return(1);
}

// removes from active list and inserts on idle list
// return valeu of 1 indicates that the connection is idle (and does not need
// a close)
int cmq_sd_cache_idle(int sd)
{
	RB *rb;
	CMQSD *csd;
	double hash;

	cmq_sd_cache_init();

	pthread_mutex_lock(&CMQ_sd_cache_lock);
	rb = RBFindI(CMQ_sd_cache_active,sd);
	if(rb == NULL) {
		pthread_mutex_unlock(&CMQ_sd_cache_lock);
		return(0);
	}
	csd = (CMQSD *)rb->value.v;
	hash = cmq_sd_cache_hash(csd->ip_str,csd->port);
	RBInsertD(CMQ_sd_cache_idle,hash,(Hval)((void *)csd));
//printf("cache idled %d\n",sd);
//fflush(stdout);
	pthread_mutex_unlock(&CMQ_sd_cache_lock);
	return(1);
}

void cmq_sd_cache_destroy(int sd)
{
	RB *rb;
	RB *drb;
	CMQSD *csd = NULL;
	double hash;

	cmq_sd_cache_init();

	pthread_mutex_lock(&CMQ_sd_cache_lock);
	rb = RBFindI(CMQ_sd_cache_active,sd);
	if(rb != NULL) {
		csd = (CMQSD *)rb->value.v;
		RBDeleteI(CMQ_sd_cache_active,rb);
		close(sd);
		free(csd);
		pthread_mutex_unlock(&CMQ_sd_cache_lock);
		return;
	}
	rb = RB_FIRST(CMQ_sd_cache_idle);
	while(rb != NULL) {
		csd = (CMQSD *)rb->value.v;
		if(csd->sd == sd) {
			hash = cmq_sd_cache_hash(csd->ip_str,csd->port);
			drb = RBFindD(CMQ_sd_cache_idle,hash);
			if(drb != NULL) {
				RBDeleteD(CMQ_sd_cache_idle,drb);
				free(csd);
				close(sd);
				pthread_mutex_lock(&CMQ_sd_cache_lock);
				return;
			}
			close(sd);
			pthread_mutex_lock(&CMQ_sd_cache_lock);
			return; // should never happen since we should find on the list
		}
		rb = rb->next;
	}
	pthread_mutex_unlock(&CMQ_sd_cache_lock);
	return;
}
