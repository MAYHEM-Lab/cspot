#include "woofc-cache.h"

#include "dlist.h"
#include "hval.h"
#include "redblack.h"
#include "woofc.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct woof_cache_element_stc {
    char name[2048];
    void* payload;
};

typedef struct woof_cache_element_stc WOOF_CACHE_EL;

void WooFCacheDelete(WOOF_CACHE* wc, DlistNode* dln);

WOOF_CACHE* WooFCacheInit(int max_size) {
    WOOF_CACHE* wc;

    wc = (WOOF_CACHE*)malloc(sizeof(WOOF_CACHE));
    if (wc == NULL) {
        return (NULL);
    }

    memset(wc, 0, sizeof(WOOF_CACHE));

    wc->rb = RBInitS();
    if (wc->rb == NULL) {
        WooFCacheFree(wc);
        return (NULL);
    }

    wc->list = DlistInit();
    if (wc->list == NULL) {
        WooFCacheFree(wc);
        return (NULL);
    }
    wc->count = 0;
    wc->max = max_size;

    pthread_mutex_init(&wc->lock, NULL);

    return (wc);
}

void WooFCacheFree(WOOF_CACHE* wc) {
    RB* rb;
    DlistNode* dln;

    if (wc == NULL) {
        return;
    }
    pthread_mutex_lock(&wc->lock);
    if (wc->rb != NULL) {
        RB_FORWARD(wc->rb, rb) {
            free(K_S(rb->key));
        }
        RBDestroyS(wc->rb);
    }
    if (wc->list != NULL) {
        DLIST_FORWARD(wc->list, dln) {
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
int WooFCacheInsert(WOOF_CACHE* wc, const char* woof_name, const void* payload) {
    char* name;
    RB* rb;
    DlistNode* dln;
    WOOF_CACHE_EL* el;

    if (wc == NULL) {
        return (-1);
    }

    if (wc->max <= 0) {
        return (-1);
    }

    name = (char*)malloc(strlen(woof_name) + 1);
    if (name == NULL) {
        return (-1);
    }

    memset(name, 0, strlen(woof_name) + 1);
    strncpy(name, woof_name, strlen(woof_name));

    el = (WOOF_CACHE_EL*)malloc(sizeof(WOOF_CACHE_EL));
    if (el == NULL) {
        free(name);
        return (-1);
    }
    memset(el, 0, sizeof(WOOF_CACHE_EL));

    pthread_mutex_lock(&wc->lock);
    /*
     * if it is full, fail out so caller can age the cache
     */
    if ((wc->count) >= wc->max) {
        pthread_mutex_unlock(&wc->lock);
        free(name);
        return (-1);
    }
    /*
     * first, see if it is in the cache.  If so, delete it
     * so this insert will replace it
     */
    rb = RBFindS(wc->rb, name);
    if (rb != NULL) {
        dln = (DlistNode*)rb->value.v;
        WooFCacheDelete(wc, dln);
    }

    el->payload = payload;
    memset(el->name, 0, sizeof(el->name));
    strncpy(el->name, name, sizeof(el->name));

    dln = DlistAppend(wc->list, (Hval)(void*)el);
    if (dln == NULL) {
        free(name);
        free(el);
        pthread_mutex_unlock(&wc->lock);
        return (-1);
    }
    RBInsertS(wc->rb, name, (Hval)(void*)dln);

    wc->count++;
    pthread_mutex_unlock(&wc->lock);

    return (1);
}

/*
 * not thread safe -- must be called inside mutual exclusion region
 */
void WooFCacheDelete(WOOF_CACHE* wc, DlistNode* dln) {
    RB* rb;
    char* str;
    WOOF_CACHE_EL* el;

    /*
     * free the string used as the rb key
     * after deleteing the node
     */
    el = (WOOF_CACHE_EL*)dln->value.v;
    rb = RBFindS(wc->rb, el->name);
    if (rb != NULL) {
        str = K_S(rb->key);
        free(str);
        RBDeleteS(wc->rb, rb);
    }

    /*
     * free the cache element
     */
    free(dln->value.v);

    /*
     * delete the node
     */
    DlistDelete(wc->list, dln);

    wc->count--;

    return;
}

void* WooFCacheFind(WOOF_CACHE* wc, const char* woof_name) {
    RB* rb;
    DlistNode* dln;
    WOOF_CACHE_EL* el;
    void* payload;

    if (wc == NULL) {
        return (NULL);
    }

    pthread_mutex_lock(&wc->lock);
    rb = RBFindS(wc->rb, woof_name);
    if (rb == NULL) {
        pthread_mutex_unlock(&wc->lock);
        return (NULL);
    }

    dln = (DlistNode*)rb->value.v;
    el = (WOOF_CACHE_EL*)dln->value.v;
    payload = el->payload;
    pthread_mutex_unlock(&wc->lock);

    return (payload);
}

void WooFCacheRemove(WOOF_CACHE* wc, const char* name) {
    RB* rb;
    DlistNode* dln;

    pthread_mutex_lock(&wc->lock);
    rb = RBFindS(wc->rb, name);
    if (rb != NULL) {
        dln = (DlistNode*)rb->value.v;
        WooFCacheDelete(wc, dln);
    }
    pthread_mutex_unlock(&wc->lock);

    return;
}

int WooFCacheFull(WOOF_CACHE* wc) {
    if (wc->count >= wc->max) {
        return (1);
    } else {
        return (0);
    }
}

void* WooFCacheAge(WOOF_CACHE* wc) {
    DlistNode* dln;
    RB* rb;
    WOOF_CACHE_EL* el;
    char* payload;

    pthread_mutex_lock(&wc->lock);
    if (WooFCacheFull(wc)) {
        dln = wc->list->first;
        el = (WOOF_CACHE_EL*)dln->value.v;
        payload = el->payload;
        WooFCacheDelete(wc, dln);
        pthread_mutex_unlock(&wc->lock);
        return (payload);
    }
    pthread_mutex_lock(&wc->lock);
    return (NULL);
}
