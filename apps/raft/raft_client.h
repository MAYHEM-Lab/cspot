#ifndef RAFT_CLIENT_H
#define RAFT_CLIENT_H

#include "raft.h"

#define DHT_CLIENT_DEFAULT_RESULT_DELAY 50

#define RAFT_SUCCESS 0
#define RAFT_ERROR -1
#define RAFT_TIMEOUT -2
#define RAFT_REDIRECTED -3
#define RAFT_PENDING -4
#define RAFT_NOT_COMMITTED -5
#define RAFT_OVERRIDEN -6

int raft_client_members;
char raft_client_replicas[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
char raft_client_leader[RAFT_NAME_LENGTH];
int raft_client_result_delay;
int raft_init_client(int members, char replicas[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
void raft_set_client_leader(char* leader);
void raft_set_client_result_delay(int delay);
uint64_t raft_sync_put(RAFT_DATA_TYPE* data, int timeout);                        // return log index
unsigned long raft_async_put(RAFT_DATA_TYPE* data);                               // return client_put request seqno
uint64_t raft_sync_get(RAFT_DATA_TYPE* data, uint64_t index, int committed_only); // return term or ERROR
uint64_t raft_async_get(RAFT_DATA_TYPE* data, unsigned long client_put_seqno);    // return term or ERROR
uint64_t raft_put_handler(char* handler, void* data, unsigned long size, int monitored, int timeout);
uint64_t raft_sessionless_put_handler(
    char* raft_leader, char* handler, void* data, unsigned long size, int monitored, int timeout);
int raft_client_put_result(uint64_t* index, uint64_t* term, unsigned long seq_no);
int raft_config_change(int members,
                       char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                       int timeout);
int raft_observe(char oberver_woof_name[RAFT_NAME_LENGTH], int timeout);
int raft_current_leader(char* woof_name, char* current_leader);
int raft_is_error(uint64_t code);
int raft_is_leader();

#endif