#ifndef WOOFC_ACCESS_H
#define WOOFC_ACCESS_H

#if defined(__cplusplus)
extern "C" {
#endif

int WooFValidURI(const char* str);
int WooFNameSpaceFromURI(const char* woof_uri_str, char* woof_namespace, int len);
int WooFNameFromURI(const char* woof_uri_str, char* woof_name, int len);
int WooFIPAddrFromURI(const char* woof_uri_str, char* woof_ip, int len);
unsigned int WooFPortHash(const char* woof_namespace);
int WooFLocalIP(char* ip_str, int len);
int WooFPortFromURI(const char* woof_uri_str, int* woof_port);

unsigned long WooFMsgPut(const char* woof_name, const char* hand_name, const void* element, unsigned long el_size);
int WooFMsgGet(const char* woof_name, void* element, unsigned long el_size, unsigned long seq_no);
unsigned long WooFMsgGetElSize(const char* woof_name);
unsigned long
WooFMsgGetLatestSeqno(const char* woof_name, const char* cause_woof_name, unsigned long cause_woof_latest_seq_no);
unsigned long WooFMsgGetTail(const char* woof_name, void* elements, unsigned long el_size, int el_count);
int WooFMsgServer(const char* woof_namespace);

unsigned long WooFPutWithCause(const char* wf_name,
                               const char* hand_name,
                               void* element,
                               unsigned long cause_host,
                               unsigned long long cause_seq_no);

int WooFURINameSpace(char* woof_uri_str, char* woof_namespace, int len);
int WooFLocalName(const char* woof_name, char* local_name, int len);

#ifdef REPAIR
unsigned long int LogGetRemoteSize(char* endpoint);
int LogGetRemote(LOG* log, MIO* mio, char* endpoint);
int WooFMsgRepair(char* woof_name, Dlist* holes);
#endif

#define USE_CMQ

#ifdef USE_CMQ
#define BACKEND "cmq"
#else
#define BACKEND "zmq"
#endif

/*
 * 90 second timeout for slow clients
 */
//#define WOOF_MSG_REQ_TIMEOUT (90000)
//#define WOOF_MSG_REQ_TIMEOUT (500)
#define WOOF_MSG_REQ_TIMEOUT (3000)

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

#if defined(__cplusplus)
#include <string>
#include <string_view>
#include <algorithm>
#include <optional>
#include <cstdint>

namespace cspot {
inline std::optional<std::string> ip_from_uri(const char* woof_uri_str) {
    char ip[25]{};
    auto err = WooFIPAddrFromURI(woof_uri_str, ip, std::size(ip));
    if (err < 0) {
        return {};
    }
    return ip;
}


inline std::optional<std::string> local_ip() {
    char ip[25]{};
    auto err = WooFLocalIP(ip, std::size(ip));
    if (err < 0) {
        return {};
    }
    return ip;
}

inline std::optional<std::string> ns_from_uri(const char* woof_uri_str) {
    char ip[1024]{};
    auto err = WooFNameSpaceFromURI(woof_uri_str, ip, std::size(ip));
    if (err < 0) {
        return {};
    }
    return ip;
}

inline std::optional<uint16_t> port_from_uri(const char* woof_uri_str) {
    int port;
    if (WooFPortFromURI(woof_uri_str, &port) < 0) {
        return {};
    }
    return static_cast<uint16_t>(port);
}
} // namespace cspot
#endif

#endif
