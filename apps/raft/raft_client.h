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

// int raft_client_members;
// char raft_client_replicas[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
// char raft_client_leader[RAFT_NAME_LENGTH];

// int raft_init_client(int members, char replicas[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
// char* raft_get_client_leader();
// void raft_set_client_leader(char* leader);
// uint64_t raft_sync_put(RAFT_DATA_TYPE* data, int timeout); // return log index
unsigned long raft_put(char* raft_leader,
                             RAFT_DATA_TYPE* data,
                             RAFT_CLIENT_PUT_OPTION* opt); // return client_put request seqno
int raft_check_committed(char* raft_leader, uint64_t index);
int raft_get(char* raft_leader, RAFT_DATA_TYPE* data, uint64_t index);
// uint64_t raft_async_get(char* raft_leader, RAFT_DATA_TYPE* data, unsigned long client_put_seqno);              //
// return term or ERROR uint64_t raft_put_handler(char* handler, void* data, unsigned long size, int monitored, int
// timeout);
uint64_t raft_put_handler(
    char* raft_leader, char* handler, void* data, unsigned long size, int monitored, RAFT_CLIENT_PUT_OPTION* opt);
// uint64_t raft_sessionless_put_handler(
//     char* raft_leader, char* handler, void* data, unsigned long size, int monitored, int timeout);
int raft_client_put_result(char* raft_leader, RAFT_CLIENT_PUT_RESULT* result, unsigned long seq_no);
// int raft_config_change(int members,
//                        char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
//                        int timeout);
// int raft_observe(char oberver_woof_name[RAFT_NAME_LENGTH], int timeout);
// int raft_current_leader(char* woof_name, char* current_leader);
int raft_is_error(uint64_t code);
int raft_is_leader();

#endif