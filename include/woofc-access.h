#ifndef WOOFC_ACCESS_H
#define WOOFC_ACCESS_H

#include "dlist.h"
#include "log.h"

#include <czmq.h>

#if defined(__cplusplus)
extern "C" {
#endif

int WooFValidURI(const char* str);
int WooFNameSpaceFromURI(const char* woof_uri_str, char* woof_namespace, int len);
int WooFNameFromURI(const char* woof_uri_str, char* woof_name, int len);
int WooFIPAddrFromURI(const char* woof_uri_str, char* woof_ip, int len);
unsigned int WooFPortHash(const char* woof_namespace);
int WooFLocalIP(char* ip_str, int len);

unsigned long WooFMsgPut(const char* woof_name, const char* hand_name, const void* element, unsigned long el_size);
int WooFMsgGet(const char* woof_name, void* element, unsigned long el_size, unsigned long seq_no);
unsigned long WooFMsgGetElSize(const char* woof_name);
unsigned long WooFMsgGetLatestSeqno(char* woof_name, char* cause_woof_name, unsigned long cause_woof_latest_seq_no);
unsigned long WooFMsgGetTail(char* woof_name, void* elements, unsigned long el_size, int el_count);
int WooFMsgServer(char* woof_namespace);

unsigned long WooFPutWithCause(
    char* wf_name, char* hand_name, void* element, unsigned long cause_host, unsigned long long cause_seq_no);

int WooFURINameSpace(char* woof_uri_str, char* woof_namespace, int len);

#ifdef REPAIR
unsigned long int LogGetRemoteSize(char* endpoint);
int LogGetRemote(LOG* log, MIO* mio, char* endpoint);
int WooFMsgRepair(char* woof_name, Dlist* holes);
#endif

/*
 * 2 minute timeout
 */
// #define WOOF_MSG_REQ_TIMEOUT (120000)
#define WOOF_MSG_REQ_TIMEOUT (500)

#define WOOF_MSG_THREADS (15)

#define WOOF_MSG_PUT (1)
#define WOOF_MSG_GET_EL_SIZE (2)
#define WOOF_MSG_GET (3)
#define WOOF_MSG_GET_TAIL (4)
#define WOOF_MSG_GET_LATEST_SEQNO (5)
#define WOOF_MSG_GET_DONE (6)
#define WOOF_MSG_REPAIR (7)
#define WOOF_MSG_REPAIR_PROGRESS (8)
#define LOG_GET_REMOTE (9)
#define LOG_GET_REMOTE_SIZE (10)

#define WOOF_MSG_CACHE_SIZE (100)

#if defined(__cplusplus)
}
#endif

#endif
