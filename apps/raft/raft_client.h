#ifndef RAFT_CLIENT_H
#define RAFT_CLIENT_H

#include "raft.h"

#define RAFT_SUCCESS 0
#define RAFT_ERROR -1
#define RAFT_TIMEOUT -2
#define RAFT_REDIRECTED -3
#define RAFT_PENDING -4
#define RAFT_NOT_COMMITTED -5
#define RAFT_OVERRIDEN -6

char raft_client_error_msg[256];
int raft_client_members;
char raft_client_servers[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
char raft_client_leader[RAFT_WOOF_NAME_LENGTH];
int raft_client_result_delay;
int raft_init_client(FILE *config);
void raft_set_client_leader(char *leader);
unsigned long raft_sync_put(RAFT_DATA_TYPE *data, int timeout); // return log index
unsigned long raft_async_put(RAFT_DATA_TYPE *data); // return client_put request seqno
unsigned long raft_sync_get(RAFT_DATA_TYPE *data, unsigned long index); // return term or ERROR
unsigned long raft_async_get(RAFT_DATA_TYPE *data, unsigned long client_put_seqno); // return term or ERROR
int raft_client_put_result(unsigned long *index, unsigned long *term, unsigned long seq_no); 
int raft_config_change(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH], int timeout);
int raft_observe(int timeout);
int raft_current_leader(char *woof_name, char *current_leader);
int raft_is_error(unsigned long code);

#endif