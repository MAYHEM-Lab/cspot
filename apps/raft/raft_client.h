#ifndef RAFT_CLIENT_H
#define RAFT_CLIENT_H

#include "raft.h"

#define DHT_CLIENT_RESULT_DELAY 100

#define RAFT_SUCCESS 0
#define RAFT_ERROR -1
#define RAFT_TIMEOUT -2
#define RAFT_REDIRECTED -3
#define RAFT_PENDING -4
#define RAFT_NOT_COMMITTED -5
#define RAFT_OVERRIDEN -6
#define RAFT_WAITING_RESULT -7

typedef struct raft_client_put_option {
    char callback_woof[RAFT_NAME_LENGTH];
    char callback_handler[RAFT_NAME_LENGTH];
    char extra_woof[RAFT_NAME_LENGTH];
    unsigned long extra_seqno;
} RAFT_CLIENT_PUT_OPTION;

unsigned long raft_put(char* raft_leader,
                       RAFT_DATA_TYPE* data,
                       RAFT_CLIENT_PUT_OPTION* opt); // return client_put request seqno
int raft_check_committed(char* raft_leader, uint64_t index);
int raft_get(char* raft_leader, RAFT_DATA_TYPE* data, uint64_t index);
uint64_t
raft_put_handler(char* raft_leader, char* handler, void* data, unsigned long size, RAFT_CLIENT_PUT_OPTION* opt);
int raft_client_put_result(char* raft_leader, RAFT_CLIENT_PUT_RESULT* result, unsigned long seq_no);
int raft_is_error(uint64_t code);
int raft_is_leader();

#endif