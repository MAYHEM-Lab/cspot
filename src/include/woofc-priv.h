#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include "dlist.h"
#include "lsema.h"
#include "mio.h"
#include "woofc.h"

struct woof_shared_stc {
    char filename[2048];
    sema mutex;
    sema tail_wait;
    unsigned long long seq_no;
    unsigned long history_size;
    unsigned long head;
    unsigned long tail;
    unsigned long element_size;
#ifdef REPAIR
    int repair_mode;
#endif
#ifdef TRACK
    int hid;
#endif
};

typedef struct woof_shared_stc WOOF_SHARED;

#ifdef REPAIR
#define NORMAL (0)
#define REPAIRING (1)
#define SHADOW (2)
#define READONLY (4)
#define REMOVE (8)
#endif

struct woof_stc {
    WOOF_SHARED* shared;
    MIO* mio;
    unsigned long ino; // for caching
};

typedef struct woof_stc WOOF;

struct element_stc {
#ifdef COMPAT1_0
    unsigned long busy;
#endif
    unsigned long long seq_no;
#ifdef TRACK
    int hid;
#endif
};

typedef struct element_stc ELID;

int WooFCreate(const char* name, unsigned long element_size, unsigned long history_size);
WOOF* WooFOpen(const char* name);
unsigned long WooFPut(const char* wf_name, const char* wf_handler, const void* element);
long unsigned int WooFPutWithCause(
    const char* wf_name, const char* hand_name, void* element, long unsigned int cause_host, unsigned long long cause_seq_no);
int WooFGet(const char* wf_name, void* element, unsigned long seq_no);
unsigned long WooFAppendWithCause(
    WOOF* wf, const char* hand_name, const void* element, unsigned long cause_host, unsigned long long cause_seq_no);
unsigned long WooFAppend(WOOF* wf, const char* hand_name, const void* element);
int WooFRead(WOOF* wf, void* element, unsigned long seq_no);
int WooFReadWithCause(
    WOOF* wf, void* element, unsigned long seq_no, unsigned long cause_host, unsigned long cause_seq_no);
int WooFReadTail(WOOF* wf, void* elements, int element_count);
int WooFReadWithCause(
    WOOF* wf, void* element, unsigned long seq_no, unsigned long cause_host, unsigned long cause_seq_no);
unsigned long WooFLatestSeqno(WOOF* wf);
unsigned long WooFLatestSeqnoWithCause(WOOF* wf,
                                       unsigned long cause_host,
                                       unsigned long long cause_seq_no,
                                       const char* cause_woof_name,
                                       unsigned long cause_woof_latest_seq_no);
unsigned long WooFEarliest(WOOF* wf);
unsigned long WooFLatest(WOOF* wf);
unsigned long WooFBack(WOOF* wf, unsigned long ndx, unsigned long elements);
unsigned long WooFForward(WOOF* wf, unsigned long ndx, unsigned long elements);
void WooFDrop(WOOF* wf);
int WooFTruncate(char* name, unsigned long seq_no);
int WooFExist(const char* name);

unsigned int WooFPortHash(const char* woof_namespace);

unsigned long WooFNameHash(const char* woof_namespace);

#ifdef REPAIR
WOOF* WooFOpenOriginal(char* name);
int WooFRepairProgress(
    char* wf_name, unsigned long cause_host, char* cause_woof, int mapping_count, unsigned long* mapping);
int WooFRepair(char* wf_name, Dlist* seq_no);
int WooFShadowCreate(
    char* name, char* original_name, unsigned long element_size, unsigned long history_size, Dlist* seq_no);
int WooFShadowForward(WOOF* wf);
int WooFReplace(WOOF* dst, WOOF* src, unsigned long ndx, unsigned long size);
unsigned long WooFIndexFromSeqno(WOOF* wf, unsigned long seq_no);
void WooFPrintMeta(FILE* fd, char* name);
void WooFDump(FILE* fd, char* name);
#endif

#define DEFAULT_WOOF_DIR "./cspot/"
#define DEFAULT_CSPOT_HOST_DIR "./cspot-host/"
#define DEFAULT_HOST_ID (0)
//#define DEFAULT_WOOF_LOG_SIZE (50000)
#define DEFAULT_WOOF_LOG_SIZE (1000000)
// #define WOOFNAMESIZE (256)

#if defined(__cplusplus)
}
#endif
