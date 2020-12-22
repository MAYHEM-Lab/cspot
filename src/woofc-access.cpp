#include "woofc-access.h"

#include "woofc.h" /* for WooFPut */

#include <arpa/inet.h>
#include <debug.h>
#include <errno.h>
#include <global.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <uriparser2.h>
#include <woofc-priv.h>

extern "C" {
int WooFValidURI(const char* str) {
    /*
     * must begin with woof://
     */
    auto prefix = strstr(str, "woof://");
    if (prefix == str) {
        return (1);
    } else {
        return (0);
    }
}

/*
 * convert URI to namespace path
 */
int WooFURINameSpace(char* woof_uri_str, char* woof_namespace, int len) {
    if (!WooFValidURI(woof_uri_str)) { /* still might be local name, but return error */
        return -1;
    }

    URI uri(woof_uri_str);
    if (uri.reserved == nullptr) {
        return -1;
    }

    if (uri.path == nullptr) {
        return -1;
    }

    strncpy(woof_namespace, uri.path, len);

    return 1;
}

/*
 * extract namespace from full woof_name
 */
int WooFNameSpaceFromURI(const char* woof_uri_str, char* woof_namespace, int len) {
    if (!WooFValidURI(woof_uri_str)) { /* still might be local name, but return error */
        return (-1);
    }

    URI uri(woof_uri_str);
    if (uri.reserved == nullptr) {
        return -1;
    }

    if (uri.path == nullptr) {
        return -1;
    }
    /*
     * walk back to the last '/' character
     */
    auto i = strlen(uri.path);
    while (i >= 0) {
        if (uri.path[i] == '/') {
            if (i <= 0) {
                return (-1);
            }
            if (i > len) { /* not enough space to hold path */
                return (-1);
            }
            strncpy(woof_namespace, uri.path, i);
            return (1);
        }
        i--;
    }
    /*
     * we didn't find a '/' in the URI path for the woofname -- error out
     */
    return (-1);
}

int WooFNameFromURI(const char* woof_uri_str, char* woof_name, int len) {
    if (!WooFValidURI(woof_uri_str)) {
        return (-1);
    }

    URI uri(woof_uri_str);
    if (uri.reserved == nullptr) {
        return -1;
    }

    if (uri.path == nullptr) {
        return -1;
    }
    /*
     * walk back to the last '/' character
     */
    auto i = strlen(uri.path);
    auto j = 0;
    /*
     * if last character in the path is a '/' this is an error
     */
    if (uri.path[i] == '/') {
        return (-1);
    }
    while (i >= 0) {
        if (uri.path[i] == '/') {
            i++;
            if (i <= 0) {
                return (-1);
            }
            if (j > len) { /* not enough space to hold path */
                return (-1);
            }
            /*
             * pull off the end as the name of the woof
             */
            strncpy(woof_name, &(uri.path[i]), len);
            return (1);
        }
        i--;
        j++;
    }
    /*
     * we didn't find a '/' in the URI path for the woofname -- error out
     */
    return (-1);
}

/*
 * returns IP address to avoid DNS issues
 */
int WooFIPAddrFromURI(const char* woof_uri_str, char* woof_ip, int len) {
    if (!WooFValidURI(woof_uri_str)) {
        return (-1);
    }

    URI uri(woof_uri_str);
    if (uri.reserved == nullptr) {
        return -1;
    }

    if (uri.host == nullptr) {
        return -1;
    }

    in_addr addr;
    /*
     * test to see if the host is an IP address already
     */
    auto err = inet_aton(uri.host, &addr);
    if (err == 1) {
        strncpy(woof_ip, uri.host, len);
        return (1);
    }

    struct hostent* he;
    /*
     * here, assume it is a DNS name
     */
    he = gethostbyname(uri.host);
    if (he == NULL) {
        return (-1);
    }

    auto list = (struct in_addr**)he->h_addr_list;
    if (list == NULL) {
        return (-1);
    }

    for (int i = 0; list[i] != NULL; i++) {
        /*
         * return first non-NULL ip address
         */
        if (list[i] != NULL) {
            strncpy(woof_ip, inet_ntoa(*list[i]), len);
            return (1);
        }
    }

    return (-1);
}

int WooFPortFromURI(const char* woof_uri_str, int* woof_port) {
    if (!WooFValidURI(woof_uri_str)) {
        return (-1);
    }

    URI uri(woof_uri_str);
    if (uri.reserved == nullptr) {
        return -1;
    }

    if (uri.port == 0) {
        return -1;
    }

    if (uri.port == -1) {
        return (-1);
    }

    *woof_port = (int)uri.port;

    return (1);
}

int WooFLocalIP(char* ip_str, int len) {
    struct ifaddrs* addrs;
    struct ifaddrs* tmp;
    char* local_ip;

    /*
     * check to see if we have been assigned a local Host_ip (say because we are in a
     * container
     */
    if (Host_ip[0] != 0) {
        strncpy(ip_str, Host_ip, len);
        return (1);
    }

    /*
     * now check to see if there is a specific ip address we should use
     * (e.g. this is a a multi-homed host and we want a specific NIC to be the NIC
     * servicing requests
     */
    local_ip = getenv("WOOFC_IP");
    if (local_ip != NULL) {
        strncpy(ip_str, local_ip, len);
        return (1);
    }

    /*
     * need the non loop back IP address for local machine to allow inter-container
     * communication (e.g. localhost won't work)
     */
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            if (strcmp(tmp->ifa_name, "lo") != 0) {
                struct sockaddr_in* pAddr = (struct sockaddr_in*)tmp->ifa_addr;
                strncpy(ip_str, inet_ntoa(pAddr->sin_addr), len);
                freeifaddrs(addrs);
                return (1);
            }
        }
        tmp = tmp->ifa_next;
    }
    fprintf(stderr, "WooFLocalIP: no local IP address found\n");
    fflush(stderr);
    freeifaddrs(addrs);
    exit(1);
}

int WooFLocalName(const char* woof_name, char* local_name, int len) {
    return (WooFNameFromURI(woof_name, local_name, len));
}

#ifdef DONEFLAG
void WooFProcessGetDone(zmsg_t* req_msg, zsock_t* receiver) {
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    char* str;

    char woof_name[2048];
    char hand_name[2048];
    char local_name[2048];
    unsigned int copy_size;
    int count;
    unsigned long seq_no;
    char buffer[255];
    int err;
    WOOF* wf;
    unsigned long done;

#ifdef DEBUG
    printf("WooFProcessGetDone: called\n");
    fflush(stdout);
#endif
    /*
     * WooFGetDone requires a woof_name, and the seq_no
     *
     * first frame is the message type
     */
    frame = zmsg_first(req_msg);
    if (frame == NULL) {
        perror("WooFProcessGetDone: couldn't set cursor in msg");
        return;
    }

    frame = zmsg_next(req_msg);
    if (frame == NULL) {
        perror("WooFProcessGetDone: couldn't find woof_name in msg");
        return;
    }
    /*
     * woof_name in the first frame
     */
    memset(woof_name, 0, sizeof(woof_name));
    str = (char*)zframe_data(frame);
    copy_size = zframe_size(frame);
    if (copy_size > (sizeof(woof_name) - 1)) {
        copy_size = sizeof(woof_name) - 1;
    }
    strncpy(woof_name, str, copy_size);

#ifdef DEBUG
    printf("WooFProcessGetDone: received woof_name %s\n", woof_name);
    fflush(stdout);
#endif
    /*
     * seq_no name in the second frame
     */
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    if (copy_size > 0) {
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, zframe_data(frame), copy_size);
        seq_no = strtoul(buffer, (char**)NULL, 10);
#ifdef DEBUG
        printf("WooFProcessGetDone: received seq_no name %lu\n", seq_no);
        fflush(stdout);
#endif
        /*
         * FIX ME: for now, all process requests are local
         */
        memset(local_name, 0, sizeof(local_name));
        err = WooFLocalName(woof_name, local_name, sizeof(local_name));
        /*
         * attempt to get the element from the local woof_name
         */
        if (err < 0) {
            wf = WooFOpen(woof_name);
        } else {
            wf = WooFOpen(local_name);
        }
        if (wf == NULL) {
            fprintf(stderr, "WooFProcessGetDone: couldn't open woof: %s\n", woof_name);
            fflush(stderr);
            done = -1;
        } else {
            done = wf->shared->done;
            WooFFree(wf);
        }
    } else { /* copy_size <= 0 */
        fprintf(stderr, "WooFProcessGetDone: no seq_no frame data for %s\n", woof_name);
        fflush(stderr);
        done = -1;
    }

    /*
     * send element back or empty frame indicating no dice
     */
    r_msg = zmsg_new();
    if (r_msg == NULL) {
        perror("WooFProcessGetDone: no reply message");
        return;
    }
#ifdef DEBUG
    printf("WooFProcessGetDone: creating frame for %lu bytes of element at seq_no %lu\n", sizeof(done), seq_no);
    fflush(stdout);
#endif

    r_frame = zframe_new(&done, sizeof(done));
    if (r_frame == NULL) {
        perror("WooFProcessGetDone: no reply frame");
        zmsg_destroy(&r_msg);
        return;
    }
    err = zmsg_append(r_msg, &r_frame);
    if (err != 0) {
        perror("WooFProcessGetDone: couldn't append to r_msg");
        zframe_destroy(&r_frame);
        zmsg_destroy(&r_msg);
        return;
    }
    err = zmsg_send(&r_msg, receiver);
    if (err != 0) {
        perror("WooFProcessGetDone: couldn't send r_msg");
        zmsg_destroy(&r_msg);
        return;
    }

    return;
}

#endif

#ifdef REPAIR
void WooFProcessRepair(zmsg_t* req_msg, zsock_t* receiver) {
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    char woof_name[2048];
    char local_name[2048];
    char* str;
    unsigned int copy_size;
    unsigned long count;
    unsigned long* seq_no;
    int err;
    char buffer[255];
    Dlist* dlist;
    DlistNode* dn;
    int i;
    Hval hval;

#ifdef DEBUG
    printf("WooFProcessRepair: called\n");
    fflush(stdout);
#endif

    /*
     * WooFProcessRepair requires a woof_name, count, and the seq_no
     *
     * first frame is the message type
     */
    frame = zmsg_first(req_msg);
    if (frame == NULL) {
        perror("WooFProcessRepair: couldn't set cursor in msg");
        return;
    }

    frame = zmsg_next(req_msg);
    if (frame == NULL) {
        perror("WooFProcessRepair: couldn't find woof_name in msg");
        return;
    }
    /*
     * woof_name in the first frame
     */
    memset(woof_name, 0, sizeof(woof_name));
    str = (char*)zframe_data(frame);
    copy_size = zframe_size(frame);
    if (copy_size > (sizeof(woof_name) - 1)) {
        copy_size = sizeof(woof_name) - 1;
    }
    strncpy(woof_name, str, copy_size);

#ifdef DEBUG
    printf("WooFProcessRepair: received woof_name %s\n", woof_name);
    fflush(stdout);
#endif
    /*
     * count in the second frame
     */
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    if (copy_size > 0) {
        count = strtoul(reinterpret_cast<const char*>(zframe_data(frame)), (char**)NULL, 10);
#ifdef DEBUG
        printf("WooFProcessRepair: received count %lu\n", count);
        fflush(stdout);
#endif
    } else { /* copy_size <= 0 */
        fprintf(stderr, "WooFProcessRepair: no count frame data for %s\n", woof_name);
        fflush(stderr);
    }

    /*
     * seq_no in the third frame
     */
    seq_no = (unsigned long*)malloc(count * sizeof(unsigned long));
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    if (copy_size > 0) {
        memcpy(seq_no, zframe_data(frame), copy_size);
#ifdef DEBUG
        printf("WooFProcessRepair: received %lu seq_nos with size %lu\n", count, copy_size);
        fflush(stdout);
#endif
    } else { /* copy_size <= 0 */
        fprintf(stderr, "WooFProcessRepair: no seq_no frame data for %s\n", woof_name);
        fflush(stderr);
    }

    dlist = DlistInit();
    if (dlist == NULL) {
        perror("WooFProcessRepair: couldn't init dlist");
        free(seq_no);
    }
    for (i = 0; i < count; i++) {
        hval.i64 = seq_no[i];
        dn = DlistAppend(dlist, hval);
        if (dn == NULL) {
            fprintf(stderr, "WooFProcessRepair: couldn't append seq_no[%d]:%lu to dlist\n", i, seq_no[i]);
            fflush(stderr);
        }
    }

    /*
     * send count back for confirmation
     */
    r_msg = zmsg_new();
    if (r_msg == NULL) {
        perror("WooFProcessRepair: no reply message");
        free(seq_no);
        DlistRemove(dlist);
        return;
    }
#ifdef DEBUG
    printf("WooFProcessRepair: creating frame for count %lu\n", count);
    fflush(stdout);
#endif

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu\0", dlist->count);
    r_frame = zframe_new(buffer, strlen(buffer));
    if (r_frame == NULL) {
        perror("WooFProcessRepair: no reply frame");
        zmsg_destroy(&r_msg);
        free(seq_no);
        DlistRemove(dlist);
        return;
    }
    err = zmsg_append(r_msg, &r_frame);
    if (err != 0) {
        perror("WooFProcessRepair: couldn't append to r_msg");
        zframe_destroy(&r_frame);
        zmsg_destroy(&r_msg);
        free(seq_no);
        DlistRemove(dlist);
        return;
    }
    err = zmsg_send(&r_msg, receiver);
    if (err != 0) {
        perror("WooFProcessRepair: couldn't send r_msg");
        zmsg_destroy(&r_msg);
        free(seq_no);
        DlistRemove(dlist);
        return;
    }
    /*
     * FIX ME: in a cloud, the calling process doesn't know the publically viewable
     * IP address so it can't determine whether the access is local or not.
     *
     * For now, assume that all Process functions convert to a local access
     */
    memset(local_name, 0, sizeof(local_name));
    err = WooFLocalName(woof_name, local_name, sizeof(local_name));
    if (err < 0) {
        strncpy(local_name, woof_name, sizeof(local_name));
    }
    err = WooFRepair(local_name, dlist);
    if (err < 0) {
        fprintf(stderr, "WooFProcessRepair: WooFRepair failed\n");
        fflush(stderr);
    }
    free(seq_no);
    DlistRemove(dlist);
    return;
}

void WooFProcessRepairProgress(zmsg_t* req_msg, zsock_t* receiver) {
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    char woof_name[2048];
    char local_name[2048];
    char cause_woof[2048];
    char* str;
    unsigned int copy_size;
    unsigned long cause_host;
    int mapping_count;
    unsigned long* mapping;
    int err;
    char buffer[255];

#ifdef DEBUG
    printf("WooFProcessRepairProgress: called\n");
    fflush(stdout);
#endif

    /*
     * WooFProcessRepairProgress requires a cause_host, cause_woof, mapping_count, and
     * mapping
     *
     * first frame is the message type
     */
    frame = zmsg_first(req_msg);
    if (frame == NULL) {
        perror("WooFProcessRepairProgress: couldn't set cursor in msg");
        return;
    }

    frame = zmsg_next(req_msg);
    if (frame == NULL) {
        perror("WooFProcessRepairProgress: couldn't find woof_name in msg");
        return;
    }
    /*
     * woof_name in the first frame
     */
    memset(woof_name, 0, sizeof(woof_name));
    str = (char*)zframe_data(frame);
    copy_size = zframe_size(frame);
    if (copy_size > (sizeof(woof_name) - 1)) {
        copy_size = sizeof(woof_name) - 1;
    }
    strncpy(woof_name, str, copy_size);

#ifdef DEBUG
    printf("WooFProcessRepairProgress: received woof_name %s\n", woof_name);
    fflush(stdout);
#endif
    /*
     * cause_host in the second frame
     */
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    if (copy_size > 0) {
        cause_host = strtoul(zframe_data(frame), (char**)NULL, 10);
#ifdef DEBUG
        printf("WooFProcessRepairProgress: received cause_host %lu\n", cause_host);
        fflush(stdout);
#endif
    } else { /* copy_size <= 0 */
        fprintf(stderr, "WooFProcessRepairProgress: no cause_host frame data for %s\n", woof_name);
        fflush(stderr);
    }

    /*
     * cause_woof in the third frame
     */
    memset(cause_woof, 0, sizeof(cause_woof));
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    str = (char*)zframe_data(frame);
    if (copy_size > (sizeof(cause_woof) - 1)) {
        copy_size = sizeof(cause_woof) - 1;
    }
    strncpy(cause_woof, str, copy_size);

    /*
     * int mapping_count in the fourth frame
     */
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    mapping_count = atoi(zframe_data(frame));
#ifdef DEBUG
    printf("WooFProcessRepairProgress: received mapping_count %d\n", mapping_count);
    fflush(stdout);
#endif

    // unsigned long *mapping)
    /*
     * mapping in the fifth frame
     */
    mapping = malloc(2 * mapping_count * sizeof(unsigned long));
    frame = zmsg_next(req_msg);
    copy_size = zframe_size(frame);
    if (copy_size > 0) {
        memcpy(mapping, zframe_data(frame), copy_size);
#ifdef DEBUG
        printf("WooFProcessRepairProgress: received mapping with size %lu\n", copy_size);
        fflush(stdout);
#endif
    } else { /* copy_size <= 0 */
        fprintf(stderr, "WooFProcessRepairProgress: no mapping frame data for %s\n", woof_name);
        fflush(stderr);
    }

    /*
     * send count back for confirmation
     */
    r_msg = zmsg_new();
    if (r_msg == NULL) {
        perror("WooFProcessRepairProgress: no reply message");
        free(mapping);
        return;
    }
#ifdef DEBUG
    printf("WooFProcessRepairProgress: creating frame for mapping_count %lu\n", mapping_count);
    fflush(stdout);
#endif

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", mapping_count);
    r_frame = zframe_new(buffer, strlen(buffer));
    if (r_frame == NULL) {
        perror("WooFProcessRepairProgress: no reply frame");
        zmsg_destroy(&r_msg);
        free(mapping);
        return;
    }
    err = zmsg_append(r_msg, &r_frame);
    if (err != 0) {
        perror("WooFProcessRepairProgress: couldn't append to r_msg");
        zframe_destroy(&r_frame);
        zmsg_destroy(&r_msg);
        free(mapping);
        return;
    }
    err = zmsg_send(&r_msg, receiver);
    if (err != 0) {
        perror("WooFProcessRepairProgress: couldn't send r_msg");
        zmsg_destroy(&r_msg);
        free(mapping);
        return;
    }
    /*
     * FIX ME: in a cloud, the calling process doesn't know the publically viewable
     * IP address so it can't determine whether the access is local or not.
     *
     * For now, assume that all Process functions convert to a local access
     */
    memset(local_name, 0, sizeof(local_name));
    err = WooFLocalName(woof_name, local_name, sizeof(local_name));
    if (err < 0) {
        strncpy(local_name, woof_name, sizeof(local_name));
    }
    err = WooFRepairProgress(local_name, cause_host, cause_woof, mapping_count, mapping);
    if (err < 0) {
        fprintf(stderr, "WooFProcessRepairProgress: WooFRepairProgress failed\n");
        fflush(stderr);
    }
    free(mapping);
    return;
}

void LogProcessGetSize(zmsg_t* req_msg, zsock_t* receiver) {
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    char* str;
    unsigned int copy_size;
    void* log_block;
    char buffer[255];
    int err;
    unsigned long int space;

#ifdef DEBUG
    printf("LogProcessGetSize: called\n");
    fflush(stdout);
#endif

    /*
     * LogProcessGetSize has no parameter frame, go straight to reply message.
     */
    r_msg = zmsg_new();
    if (r_msg == NULL) {
        perror("LogProcessGetSize: no reply message");
        return;
    }

    log_block = (void*)Name_log;
    space = Name_log->size * sizeof(EVENT) + sizeof(LOG);
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu\0", space);
#ifdef DEBUG
    printf("LogProcessGetSize: creating frame for log size\n");
    fflush(stdout);
#endif
    r_frame = zframe_new(buffer, strlen(buffer));
    if (r_frame == NULL) {
        perror("LogProcessGetSize: no reply frame");
        zmsg_destroy(&r_msg);
        return;
    }

    err = zmsg_append(r_msg, &r_frame);
    if (err != 0) {
        perror("LogProcessGetSize: couldn't append to r_msg");
        zframe_destroy(&r_frame);
        zmsg_destroy(&r_msg);
        return;
    }
    err = zmsg_send(&r_msg, receiver);
    if (err != 0) {
        perror("LogProcessGetSize: couldn't send r_msg");
        zmsg_destroy(&r_msg);
        return;
    }
    return;
}

void LogProcessGet(zmsg_t* req_msg, zsock_t* receiver) {
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    char* str;
    unsigned int copy_size;
    void* log_block;
    char buffer[255];
    int err;
    unsigned long int space;

#ifdef DEBUG
    printf("LogProcessGet: called\n");
    fflush(stdout);
#endif

    /*
     * LogProcessGet has no parameter frame, go straight to reply message.
     */
    r_msg = zmsg_new();
    if (r_msg == NULL) {
        perror("LogProcessGet: no reply message");
        return;
    }

    log_block = (void*)Name_log;
    space = Name_log->size * sizeof(EVENT) + sizeof(LOG);

#ifdef DEBUG
    printf("LogProcessGet: creating frame for %lu bytes of log_block\n", space);
    LogPrint(stdout, Name_log);
    fflush(stdout);
#endif
    r_frame = zframe_new(log_block, space);
    if (r_frame == NULL) {
        perror("LogProcessGet: no reply frame");
        free(log_block);
        zmsg_destroy(&r_msg);
        return;
    }

    err = zmsg_append(r_msg, &r_frame);
    if (err != 0) {
        perror("LogProcessGet: couldn't append to r_msg");
        zframe_destroy(&r_frame);
        zmsg_destroy(&r_msg);
        return;
    }
    err = zmsg_send(&r_msg, receiver);
    if (err != 0) {
        perror("LogProcessGet: couldn't send r_msg");
        zmsg_destroy(&r_msg);
        return;
    }
    log_block = NULL;
    return;
}
#endif

#ifdef DONEFLAG
int WooFMsgGetDone(char* woof_name, unsigned long seq_no) {
    char endpoint[255];
    char namespace[2048];
    char ip_str[25];
    int port;
    zmsg_t* msg;
    zmsg_t* r_msg;
    int r_size;
    zframe_t* frame;
    zframe_t* r_frame;
    char buffer[255];
    char* str;
    struct timeval tm;
    int err;
    unsigned long done;

    memset(namespace, 0, sizeof(namespace));
    err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
    if (err < 0) {
        fprintf(stderr, "WooFMsgGetDone: woof: %s no name space\n", woof_name);
        fflush(stderr);
        return (-1);
    }

    memset(ip_str, 0, sizeof(ip_str));
    err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
    if (err < 0) {
        /*
         * assume it is local
         */
        err = WooFLocalIP(ip_str, sizeof(ip_str));
        if (err < 0) {
            fprintf(stderr, "WooFMsgGet: woof: %s invalid IP address\n", woof_name);
            fflush(stderr);
            return (-1);
        }
    }

    err = WooFPortFromURI(woof_name, &port);
    if (err < 0) {
        port = WooFPortHash(namespace);
    }

    memset(endpoint, 0, sizeof(endpoint));
    sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
    printf("WooFMsgGetDone: woof: %s trying enpoint %s\n", woof_name, endpoint);
    fflush(stdout);
#endif

    msg = zmsg_new();
    if (msg == NULL) {
        fprintf(stderr, "WooFMsgGetDone: woof: %s no outbound msg to server at %s\n", woof_name, endpoint);
        perror("WooFMsgGet: allocating msg");
        fflush(stderr);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgGet: woof: %s got new msg\n", woof_name);
    fflush(stdout);
#endif

    /*
     * this is a get done flag message
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%02lu", WOOF_MSG_GET_DONE);
    frame = zframe_new(buffer, strlen(buffer));
    if (frame == NULL) {
        fprintf(stderr,
                "WooFMsgGetDone: woof: %s no frame for WOOF_MSG_GET_DONE command in to "
                "server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgGetDone: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }

#ifdef DEBUG
    printf("WooFMsgGet: woof: %s got WOOF_MSG_GET_DONE command frame frame\n", woof_name);
    fflush(stdout);
#endif
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgGetDone: woof: %s can't append WOOF_MSG_GET_DONE command frame "
                "to msg for server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgGetDone: couldn't append woof_name frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }

    /*
     * make a frame for the woof_name
     */
    frame = zframe_new(woof_name, strlen(woof_name));
    if (frame == NULL) {
        fprintf(stderr, "WooFMsgGetDone: woof: %s no frame for woof_name to server at %s\n", woof_name, endpoint);
        perror("WooFMsgGet: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgGetDone: woof: %s got woof_name namespace frame\n", woof_name);
    fflush(stdout);
#endif
    /*
     * add the woof_name frame to the msg
     */
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(
            stderr, "WooFMsgGetDone: woof: %s can't append woof_name to frame to server at %s\n", woof_name, endpoint);
        perror("WooFMsgGetDone: couldn't append woof_name namespace frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgGetDone: woof: %s added woof_name namespace to frame\n", woof_name);
    fflush(stdout);
#endif

    /*
     * make a frame for the seq_no
     */

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu\0", seq_no);
    frame = zframe_new(buffer, strlen(buffer));

    if (frame == NULL) {
        fprintf(stderr, "WooFMsgGetDone: woof: %s no frame for seq_no %lu server at %s\n", woof_name, seq_no, endpoint);
        perror("WooFMsgGetDone: couldn't get new seq_no frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgGetDone: woof: %s got frame for seq_no %lu\n", woof_name, seq_no);
    fflush(stdout);
#endif

    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgGetDone: woof: %s couldn't append frame for seq_no %lu to server "
                "at %s\n",
                woof_name,
                seq_no,
                endpoint);
        perror("WooFMsgGetDone: couldn't append seq_no frame");
        fflush(stderr);
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgGetDone: woof: %s appended frame for seq_no %lu\n", woof_name, seq_no);
    fflush(stdout);
#endif

#ifdef DEBUG
    printf("WooFMsgGetDone: woof: %s sending message to server at %s\n", woof_name, endpoint);
    fflush(stdout);
#endif

    r_msg = ServerRequest(endpoint, msg);
    if (r_msg == NULL) {
        fprintf(stderr,
                "WooFMsgGetDone: woof: %s couldn't recv msg for seq_no %lu name %s to "
                "server at %s\n",
                woof_name,
                seq_no,
                endpoint);
        perror("WooFMsgGetDone: no response received");
        fflush(stderr);
        return (-1);
    } else {
        r_frame = zmsg_first(r_msg);
        if (r_frame == NULL) {
            fprintf(stderr,
                    "WooFMsgGetDone: woof: %s no recv frame for seq_no %lu to server at %s\n",
                    woof_name,
                    seq_no,
                    endpoint);
            perror("WooFMsgGetDone: no response frame");
            zmsg_destroy(&r_msg);
            return (-1);
        }
        r_size = zframe_size(r_frame);
        if (r_size == 0) {
            fprintf(stderr,
                    "WooFMsgGetDone: woof: %s no data in frame for seq_no %lu to server "
                    "at %s\n",
                    woof_name,
                    seq_no,
                    endpoint);
            fflush(stderr);
            zmsg_destroy(&r_msg);
            return (-1);
        } else {
            done = 0;
            memcpy(&done, zframe_data(r_frame), sizeof(done));
#ifdef DEBUG
            printf("WooFMsgGetDone: woof: %s got done: %lu for seq_no %lu message to "
                   "server at %s\n",
                   woof_name,
                   done,
                   seq_no,
                   endpoint);
            fflush(stdout);

#endif
            zmsg_destroy(&r_msg);
        }
    }

    if (done == 0) {
        return (0);
    } else if (done == 1) {
        return (1);
    } else {
        fprintf(stderr, "WooFMsgGetDone: bad done value from %s, seq_no %lu, done: %lu\n", woof_name, seq_no, done);
        fflush(stderr);
        return (-1);
    }
}

#endif

#ifdef REPAIR
unsigned long int LogGetRemoteSize(char* endpoint) {
    int err;
    zmsg_t* msg;
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    unsigned long copy_size;
    char buffer[255];
    unsigned long int size;

    if (strcmp(endpoint, "local") == 0) {
        return Name_log->size * sizeof(EVENT) + sizeof(LOG);
    }
    msg = zmsg_new();
    if (msg == NULL) {
        fprintf(stderr, "LogGetRemoteSize: no outbound msg to server at %s\n", endpoint);
        perror("LogGetRemoteSize: allocating msg");
        fflush(stderr);
        return (-1);
    }
#ifdef DEBUG
    printf("LogGetRemoteSize: got new msg\n");
    fflush(stdout);
#endif

    /*
     * this is a LOG_GET_REMOTE_SIZE message
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%02lu", LOG_GET_REMOTE_SIZE);
    frame = zframe_new(buffer, strlen(buffer));
    if (frame == NULL) {
        fprintf(stderr,
                "LogGetRemoteSize: no frame for LOG_GET_REMOTE_SIZE command in to server "
                "at %s\n",
                endpoint);
        perror("LogGetRemoteSize: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("LogGetRemoteSize: got LOG_GET_REMOTE_SIZE command frame frame\n");
    fflush(stdout);
#endif
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "LogGetRemoteSize: can't append LOG_GET_REMOTE_SIZE command frame to msg "
                "for server at %s\n",
                endpoint);
        perror("LogGetRemoteSize: couldn't append command frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }

    /*
     * get response
     */
    r_msg = ServerRequest(endpoint, msg);
    if (r_msg == NULL) {
        fprintf(stderr, "LogGetRemoteSize: couldn't recv msg for log tail from server at %s\n", endpoint);
        perror("LogGetRemoteSize: no response received");
        fflush(stderr);
        return (-1);
    }
    r_frame = zmsg_first(r_msg);
    if (r_frame == NULL) {
        fprintf(stderr, "LogGetRemoteSize: no recv frame for log tail from server at %s\n", endpoint);
        perror("LogGetRemoteSize: no response frame");
        zmsg_destroy(&r_msg);
        return (-1);
    }
    size = strtoul(zframe_data(r_frame), (char**)NULL, 10);
    zmsg_destroy(&r_msg);

#ifdef DEBUG
    printf("LogGetRemoteSize: received log size from %s\n", endpoint);
    fflush(stdout);
#endif

    return size;
}

int LogGetRemote(LOG* log, MIO* mio, char* endpoint) {
    int err;
    zmsg_t* msg;
    zmsg_t* r_msg;
    zframe_t* frame;
    zframe_t* r_frame;
    unsigned long copy_size;
    char buffer[255];

    if (strcmp(endpoint, "local") == 0) {
        unsigned long space = Name_log->size * sizeof(EVENT) + sizeof(LOG);
        memcpy((void*)log, (void*)Name_log, space);
        log->m_buf = mio;
        return (1);
    }

    msg = zmsg_new();
    if (msg == NULL) {
        fprintf(stderr, "LogGetRemote: no outbound msg to server at %s\n", endpoint);
        perror("LogGetRemote: allocating msg");
        fflush(stderr);
        return (-1);
    }
#ifdef DEBUG
    printf("LogGetRemote: got new msg\n");
    fflush(stdout);
#endif

    /*
     * this is a LOG_GET_REMOTE message
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%02lu", LOG_GET_REMOTE);
    frame = zframe_new(buffer, strlen(buffer));
    if (frame == NULL) {
        fprintf(stderr, "LogGetRemote: no frame for LOG_GET_REMOTE command in to server at %s\n", endpoint);
        perror("LogGetRemote: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("LogGetRemote: got LOG_GET_REMOTE command frame frame\n");
    fflush(stdout);
#endif
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "LogGetRemote: can't append LOG_GET_REMOTE command frame to msg for "
                "server at %s\n",
                endpoint);
        perror("LogGetRemote: couldn't append command frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }

    /*
     * get response
     */
    r_msg = ServerRequest(endpoint, msg);
    if (r_msg == NULL) {
        fprintf(stderr, "LogGetRemote: couldn't recv msg for log tail from server at %s\n", endpoint);
        perror("LogGetRemote: no response received");
        fflush(stderr);
        return (-1);
    }
    r_frame = zmsg_first(r_msg);
    if (r_frame == NULL) {
        fprintf(stderr, "LogGetRemote: no recv frame for log tail from server at %s\n", endpoint);
        perror("LogGetRemote: no response frame");
        zmsg_destroy(&r_msg);
        return (-1);
    }
    copy_size = zframe_size(r_frame);
    memcpy((void*)log, (void*)zframe_data(r_frame), copy_size);
    log->m_buf = mio;
    zmsg_destroy(&r_msg);

#ifdef DEBUG
    printf("LogGetRemote: received log from %s\n", endpoint);
    fflush(stdout);
#endif

    return (1);
}

// int LogGetRemoteLite(LOG *log, unsigned long size)
// {
// 	int sockfd, n;
// 	struct sockaddr_in serv_addr;
// 	struct hostent *server;
// 	cw_pack_context pc;
// 	char buffer[4];
// 	char *recv_buf;

// 	cw_pack_context_init(&pc, buffer, 4, 0, 0);
// 	cw_pack_unsigned(&pc, 9);
// 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sockfd < 0)
// 	{
// 		return (0);
// 	}
// 	server = gethostbyname("192.168.0.111");
// 	if (server == NULL)
// 	{
// 		return (0);
// 	}
// 	bzero((char *)&serv_addr, sizeof(serv_addr));
// 	serv_addr.sin_family = AF_INET;
// 	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
// server->h_length); 	serv_addr.sin_port = htons(9993); 	if (connect(sockfd, (struct
// sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
// 	{
// 		return (0);
// 	}
// 	n = write(sockfd, buffer, strlen(buffer));
// 	if (n < 0)
// 	{
// 		return (0);
// 	}
// 	recv_buf = malloc(size);
// 	n = read(sockfd, recv_buf, size);
// 	if (n < 0)
// 	{
// 		return (0);
// 	}

// 	return (1);
// }

/*
 * Start woof repair. The woof objects from begin_seq_no to end_seq_no are to be repaired.
 */
int WooFMsgRepair(char* woof_name, Dlist* holes) {
    char endpoint[255];
    char namespace[2048];
    char ip_str[25];
    int port;
    zmsg_t* msg;
    zmsg_t* r_msg;
    int r_size;
    zframe_t* frame;
    zframe_t* r_frame;
    char buffer[255];
    char* str;
    unsigned long to_be_filled;
    unsigned long count;
    unsigned long* seq_no;
    DlistNode* dn;
    int i;
    struct timeval tm;
    int err;

    if (holes == NULL) {
        fprintf(stderr, "WooFMsgRepair: hole list is NULL\n");
        fflush(stderr);
        return (-1);
    }
    count = holes->count;
    seq_no = malloc(count * sizeof(unsigned long));
    i = 0;
    DLIST_FORWARD(holes, dn) {
        seq_no[i++] = dn->value.i64;
    }

    memset(namespace, 0, sizeof(namespace));
    err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
    if (err < 0) {
        fprintf(stderr, "WooFMsgRepair: woof: %s no name space\n", woof_name);
        fflush(stderr);
        return (-1);
    }

    memset(ip_str, 0, sizeof(ip_str));
    err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
    if (err < 0) {
        /*
         * assume it is local
         */
        err = WooFLocalIP(ip_str, sizeof(ip_str));
        if (err < 0) {
            fprintf(stderr, "WooFMsgRepair: woof: %s invalid IP address\n", woof_name);
            fflush(stderr);
            return (-1);
        }
    }

    err = WooFPortFromURI(woof_name, &port);
    if (err < 0) {
        port = WooFPortHash(namespace);
    }

    memset(endpoint, 0, sizeof(endpoint));
    sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s trying enpoint %s\n", woof_name, endpoint);
    fflush(stdout);
#endif

    msg = zmsg_new();
    if (msg == NULL) {
        fprintf(stderr, "WooFMsgRepair: woof: %s no outbound msg to server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepair: allocating msg");
        fflush(stderr);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s got new msg\n", woof_name);
    fflush(stdout);
#endif

    /*
     * this is a repair message
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%02lu", WOOF_MSG_REPAIR);
    frame = zframe_new(buffer, strlen(buffer));
    if (frame == NULL) {
        fprintf(stderr,
                "WooFMsgRepair: woof: %s no frame for WOOF_MSG_REPAIR command in to "
                "server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgRepair: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }

#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s got WOOF_MSG_REPAIR command frame frame\n", woof_name);
    fflush(stdout);
#endif
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepair: woof: %s can't append WOOF_MSG_REPAIR command frame to "
                "msg for server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgRepair: couldn't append WOOF_MSG_REPAIR command frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }

    /*
     * make a frame for the woof_name
     */
    frame = zframe_new(woof_name, strlen(woof_name));
    if (frame == NULL) {
        fprintf(stderr, "WooFMsgRepair: woof: %s no frame for woof_name to server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepair: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s got woof_name namespace frame\n", woof_name);
    fflush(stdout);
#endif
    /*
     * add the woof_name frame to the msg
     */
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(
            stderr, "WooFMsgRepair: woof: %s can't append woof_name to frame to server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepair: couldn't append woof_name namespace frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s added woof_name namespace to frame\n", woof_name);
    fflush(stdout);
#endif

    /*
     * make a frame for the count
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu\0", count);
    frame = zframe_new(buffer, strlen(buffer));

    if (frame == NULL) {
        fprintf(stderr, "WooFMsgRepair: woof: %s no frame for count %lu server at %s\n", woof_name, count, endpoint);
        perror("WooFMsgRepair: couldn't get new count frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s got frame for count %lu\n", woof_name, count);
    fflush(stdout);
#endif

    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepair: woof: %s couldn't append frame for count %lu to server "
                "at %s\n",
                woof_name,
                count,
                endpoint);
        perror("WooFMsgRepair: couldn't append count frame");
        fflush(stderr);
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s appended frame for count %lu\n", woof_name, count);
    fflush(stdout);
#endif

    /*
     * make a frame for the seq_no
     */
    frame = zframe_new(seq_no, count * sizeof(unsigned long));

    if (frame == NULL) {
        fprintf(stderr, "WooFMsgRepair: woof: %s no frame for seq_no server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepair: couldn't get new seq_no frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s got frame for seq_no\n", woof_name);
    fflush(stdout);
#endif

    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(
            stderr, "WooFMsgRepair: woof: %s couldn't append frame for seq_no to server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepair: couldn't append seq_no frame");
        fflush(stderr);
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s appended frame for seq_no\n", woof_name);
    fflush(stdout);
#endif

#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s sending message to server at %s\n", woof_name, endpoint);
    fflush(stdout);
#endif

    r_msg = ServerRequest(endpoint, msg);
    if (r_msg == NULL) {
        fprintf(stderr, "WooFMsgRepair: woof: %s couldn't recv msg from server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepair: no response received");
        fflush(stderr);
        return (-1);
    } else {
        r_frame = zmsg_first(r_msg);
        if (r_frame == NULL) {
            fprintf(stderr, "WooFMsgRepair: woof: %s no recv frame from server at %s\n", woof_name, endpoint);
            perror("WooFMsgRepair: no response frame");
            zmsg_destroy(&r_msg);
            return (-1);
        }
        str = zframe_data(r_frame);
        to_be_filled = strtoul(str, (char**)NULL, 10);
        zmsg_destroy(&r_msg);
    }

#ifdef DEBUG
    printf("WooFMsgRepair: woof: %s recvd to_be_filled %lu message from server at %s\n",
           woof_name,
           to_be_filled,
           endpoint);
    fflush(stdout);
#endif
    if (to_be_filled != count) {
        return (-1);
    }
    return (1);
}

/*
 * Start woof repair. The woof objects from begin_seq_no to end_seq_no are to be repaired.
 */
int WooFMsgRepairProgress(
    char* woof_name, unsigned long cause_host, char* cause_woof, int mapping_count, unsigned long* mapping) {
    char endpoint[255];
    char namespace[2048];
    char ip_str[25];
    int port;
    zmsg_t* msg;
    zmsg_t* r_msg;
    int r_size;
    zframe_t* frame;
    zframe_t* r_frame;
    char buffer[255];
    char* str;
    unsigned long count;
    int i;
    struct timeval tm;
    int err;

#ifdef DEBUG
    printf("WooFMsgRepairProgress: called\n");
    fflush(stdout);
#endif

    if (cause_woof == NULL || mapping == NULL) {
        fprintf(stderr, "WooFMsgRepairProgress: invalid parameter\n");
        fflush(stderr);
        return (-1);
    }

    memset(namespace, 0, sizeof(namespace));
    err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
    if (err < 0) {
        fprintf(stderr, "WooFMsgRepairProgress: woof: %s no name space\n", woof_name);
        fflush(stderr);
        return (-1);
    }

    memset(ip_str, 0, sizeof(ip_str));
    err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
    if (err < 0) {
        /*
         * assume it is local
         */
        err = WooFLocalIP(ip_str, sizeof(ip_str));
        if (err < 0) {
            fprintf(stderr, "WooFMsgRepairProgress: woof: %s invalid IP address\n", woof_name);
            fflush(stderr);
            return (-1);
        }
    }

    err = WooFPortFromURI(woof_name, &port);
    if (err < 0) {
        port = WooFPortHash(namespace);
    }

    memset(endpoint, 0, sizeof(endpoint));
    sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s trying enpoint %s\n", woof_name, endpoint);
    fflush(stdout);
#endif

    msg = zmsg_new();
    if (msg == NULL) {
        fprintf(stderr, "WooFMsgRepairProgress: woof: %s no outbound msg to server at %s\n", woof_name, endpoint);
        fflush(stderr);
        perror("WooFMsgRepairProgress: allocating msg");
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got new msg\n", woof_name);
    fflush(stdout);
#endif

    /*
     * this is a WOOF_MSG_REPAIR_PROGRESS message
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%02lu", WOOF_MSG_REPAIR_PROGRESS);
    frame = zframe_new(buffer, strlen(buffer));
    if (frame == NULL) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s no frame for WOOF_MSG_REPAIR_PROGRESS "
                "command in to server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }

#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got WOOF_MSG_REPAIR_PROGRESS command frame "
           "frame\n",
           woof_name);
    fflush(stdout);
#endif
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s can't append WOOF_MSG_REPAIR_PROGRESS "
                "command frame to msg for server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't append WOOF_MSG_REPAIR_PROGRESS command "
               "frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }

    /*
     * make a frame for the woof_name
     */
    frame = zframe_new(woof_name, strlen(woof_name));
    if (frame == NULL) {
        fprintf(
            stderr, "WooFMsgRepairProgress: woof: %s no frame for woof_name to server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepairProgress: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got woof_name namespace frame\n", woof_name);
    fflush(stdout);
#endif
    /*
     * add the woof_name frame to the msg
     */
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s can't append woof_name to frame to "
                "server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't append woof_name namespace frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s added woof_name namespace to frame\n", woof_name);
    fflush(stdout);
#endif

    /*
     * make a frame for the cause_host
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%lu\0", cause_host);
    frame = zframe_new(buffer, strlen(buffer));

    if (frame == NULL) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s no frame for cause_host %lu server at %s\n",
                woof_name,
                cause_host,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't get new cause_host frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got frame for cause_host %lu\n", woof_name, cause_host);
    fflush(stdout);
#endif

    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s couldn't append frame for cause_host "
                "%lu to server at %s\n",
                woof_name,
                cause_host,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't append cause_host frame");
        fflush(stderr);
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s appended frame for cause_host %lu\n", woof_name, cause_host);
    fflush(stdout);
#endif

    /*
     * make a frame for the cause_woof
     */
    frame = zframe_new(cause_woof, strlen(cause_woof));
    if (frame == NULL) {
        fprintf(
            stderr, "WooFMsgRepairProgress: woof: %s no frame for cause_woof to server at %s\n", cause_woof, endpoint);
        perror("WooFMsgRepairProgress: couldn't get new frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got cause_woof namespace frame\n", cause_woof);
    fflush(stdout);
#endif
    /*
     * add the cause_woof frame to the msg
     */
    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s can't append cause_woof to frame to "
                "server at %s\n",
                cause_woof,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't append cause_woof namespace frame");
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s added cause_woof namespace to frame\n", cause_woof);
    fflush(stdout);
#endif

    /*
     * make a frame for the mapping_count
     */
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", mapping_count);
    frame = zframe_new(buffer, strlen(buffer));

    if (frame == NULL) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s no frame for mapping_count %d server at "
                "%s\n",
                woof_name,
                mapping_count,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't get new mapping_count frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got frame for mapping_count %d\n", woof_name, mapping_count);
    fflush(stdout);
#endif

    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s couldn't append frame for mapping_count "
                "%d to server at %s\n",
                woof_name,
                mapping_count,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't append mapping_count frame");
        fflush(stderr);
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s appended frame for mapping_count %d\n", woof_name, mapping_count);
    fflush(stdout);
#endif

    /*
     * make a frame for the mapping
     */
    frame = zframe_new(mapping, 2 * mapping_count * sizeof(unsigned long));

    if (frame == NULL) {
        fprintf(stderr, "WooFMsgRepairProgress: woof: %s no frame for mapping server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepairProgress: couldn't get new mapping frame");
        fflush(stderr);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s got frame for mapping\n", woof_name);
    fflush(stdout);
#endif

    err = zmsg_append(msg, &frame);
    if (err < 0) {
        fprintf(stderr,
                "WooFMsgRepairProgress: woof: %s couldn't append frame for mapping to "
                "server at %s\n",
                woof_name,
                endpoint);
        perror("WooFMsgRepairProgress: couldn't append mapping frame");
        fflush(stderr);
        zframe_destroy(&frame);
        zmsg_destroy(&msg);
        return (-1);
    }
#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s appended frame for mapping\n", woof_name);
    fflush(stdout);
#endif

#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s sending message to server at %s\n", woof_name, endpoint);
    fflush(stdout);
#endif

    r_msg = ServerRequest(endpoint, msg);
    if (r_msg == NULL) {
        fprintf(stderr, "WooFMsgRepairProgress: woof: %s couldn't recv msg from server at %s\n", woof_name, endpoint);
        perror("WooFMsgRepairProgress: no response received");
        fflush(stderr);
        return (-1);
    } else {
        r_frame = zmsg_first(r_msg);
        if (r_frame == NULL) {
            fprintf(stderr, "WooFMsgRepairProgress: woof: %s no recv frame from server at %s\n", woof_name, endpoint);
            perror("WooFMsgRepairProgress: no response frame");
            zmsg_destroy(&r_msg);
            return (-1);
        }
        str = zframe_data(r_frame);
        count = strtoul(str, (char**)NULL, 10);
        zmsg_destroy(&r_msg);
    }

#ifdef DEBUG
    printf("WooFMsgRepairProgress: woof: %s recvd count %lu message from server at %s\n", woof_name, count, endpoint);
    fflush(stdout);
#endif
    if (count != mapping_count) {
        return (-1);
    }
    return (1);
}

#endif
}