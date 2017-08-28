#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "woofc.h"
#include "woofc-cache.h"

#include "hval.h"
#include "dlist.h"
#include "redblack.h"

struct woof_cache_element_stc
{
	char *name;
	void *payload;
};

typedef struct woof_cache_element_stc WOOF_CACHE_EL;

void WooFCacheDelete(WOOF_CACHE *wc, DlistNode *dln);


WOOF_CACHE *WooFCacheInit(int max_size)
{
	WOOF_CACHE *wc;

	wc = (WOOF_CACHE *)malloc(sizeof(WOOF_CACHE));
	if(wc == NULL) {
		return(NULL);
	}

	memset(wc,0,sizeof(WOOF_CACHE));

	wc->rb = RBInitS();
	if(wc->rb == NULL) {
		WooFCacheFree(wc);
		return(NULL);
	}

	wc->list = DlistInit();
	if(wc->list == NULL) {
		WooFCacheFree(wc);
		return(NULL);
	}
	wc->count = 0;
	wc->max = max_size;

	pthread_mutex_init(&wc->lock,NULL);

	return(wc);
}

void WooFCacheFree(WOOF_CACHE *wc)
{
	RB *rb;
	DlistNode *dln;

	if(wc == NULL) {
		return;
	}
	pthread_mutex_lock(&wc->lock);
	if(wc->rb != NULL) {
		RB_FORWARD(wc->rb,rb) {
			free(K_S(rb->key));
		}
		RBDestroyS(wc->rb);
	}
	if(wc->list != NULL) {
		DLIST_FORWARD(wc->list,dln) {
			free(dln->value.v);
		}
		DlistRemove(wc->list);
	}

	pthread_mutex_unlock(&wc->lock);
	free(wc);

	return;
}

/*
 * take an arbitrary payload because what we want to cache for a WOOF may differ
 */
int WooFCacheInsert(WOOF_CACHE *wc, char *woof_name, void *payload)
{
	char *name;
	RB *rb;
	DlistNode *dln;
	WOOF_CACHE_EL *el;

	if(wc == NULL) {
		return(-1);
	}

	if(wc->max <= 0) {
		return(-1);
	}

	name = (char *)malloc(strlen(woof_name)+1);
	if(name == NULL) {
		return(-1);
	}

	memset(name,0,strlen(woof_name)+1);
	strncpy(name,woof_name,strlen(woof_name));

	el = (WOOF_CACHE_EL *)malloc(sizeof(WOOF_CACHE_EL));
	if(el == NULL) {
		free(name);
		return(-1);
	}
	memset(el,0,sizeof(WOOF_CACHE_EL));
	el->name = name;

	pthread_mutex_lock(&wc->lock);
	/*
	 * first, see if it is in the cache.  If so, delete it
	 * so this insert will replace it
	 */
	rb = RBFindS(wc->rb,name);
	if(rb != NULL) {
		dln = (DlistNode *)rb->value.v;
		WooFCacheDelete(wc,dln);
	}
	

	/*
	 * if this insert would exceed max, delete oldest in the list
	 */
	if((wc->count+1) >= wc->max) {
		WooFCacheDelete(wc,wc->list->first);
	}

	el->payload = payload;
	dln = DlistAppend(wc->list,(Hval)(void *)el);
	if(dln == NULL) {
		free(name);
		free(el);
		pthread_mutex_unlock(&wc->lock);
		return(-1);
	}
	RBInsertS(wc->rb,name,(Hval)(void *)dln);

	wc->count++;
	pthread_mutex_unlock(&wc->lock);

	return(1);
}

/*
 * not thread safe -- must be called inside mutual exclusion region
 */
void WooFCacheDelete(WOOF_CACHE *wc, DlistNode *dln)
{
	RB *rb;
	char *str;
	WOOF_CACHE_EL *el;

	el = (WOOF_CACHE_EL *)dln->value.v;
	/*
	 * find the rb node
	 */
	rb = RBFindS(wc->rb,el->name);
	if(rb != NULL) {
		RBDeleteS(wc->rb,rb);
	}
	free(el->name);
	free(el);

	/*
	 * delete the node
	 */
	DlistDelete(wc->list,dln);


	return;
}

void *WooFCacheFind(WOOF_CACHE *wc, char *woof_name)
{
	RB *rb;
	DlistNode *dln;
	WOOF_CACHE_EL *el;
	void *payload;

	if(wc == NULL) {
		return(NULL);
	}

	pthread_mutex_lock(&wc->lock);
	rb = RBFindS(wc->rb,woof_name);
	if(rb == NULL) {
		pthread_mutex_unlock(&wc->lock);
		return(NULL);
	}

	dln = (DlistNode *)rb->value.v;
	el = (WOOF_CACHE_EL *)dln->value.v;
	payload = el->payload;
	pthread_mutex_unlock(&wc->lock);

	return(payload);
}

	
	
void WooFCacheRemove(WOOF_CACHE *wc, char *name)
{
	RB *rb;
	DlistNode *dln;

	pthread_mutex_lock(&wc->lock);
        rb = RBFindS(wc->rb,name);
        if(rb != NULL) {
                dln = (DlistNode *)rb->value.v;
                WooFCacheDelete(wc,dln);
        }
	pthread_mutex_unlock(&wc->lock);


	return;
}
