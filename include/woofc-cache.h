#ifndef WOOFC_CACHE_H
#define WOOFC_CACHE_H

#include "dlist.h"
#include "hval.h"
#include "pthread.h"
#include "redblack.h"

struct woof_cache_stc {
    pthread_mutex_t lock;
    RB* rb;
    Dlist* list;
    int count;
    int max;
};

typedef struct woof_cache_stc WOOF_CACHE;

WOOF_CACHE* WooFCacheInit(int max_size);
void WooFCacheFree(WOOF_CACHE* wc);
int WooFCacheInsert(WOOF_CACHE* wc, char* woof_name, void* payload);
void* WooFCacheFind(WOOF_CACHE* wc, char* woof_name);
void WooFCacheRemove(WOOF_CACHE* wc, char* name);
int WooFCacheFull(WOOF_CACHE* wc);
void* WooFCacheAge(WOOF_CACHE* wc);

#endif
