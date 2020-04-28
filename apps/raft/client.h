#ifndef RAFT_CLIENT_H
#define RAFT_CLIENT_H

#include "raft.h"

#define RAFT_NOT_COMMITTED 2
#define RAFT_PENDING 1
#define RAFT_SUCCESS 0
#define RAFT_ERROR -1
#define RAFT_TIMEOUT -2
#define RAFT_REDIRECTED -3

int raft_client_members;
char raft_client_servers[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
char raft_client_leader[RAFT_WOOF_NAME_LENGTH];
int raft_client_timeout;
int raft_client_result_delay;
int raft_init_client(FILE *fp);
void raft_set_client_leader(char *leader);
void raft_set_client_timeout(int timeout);
int raft_put(RAFT_DATA_TYPE *data, unsigned long *index, unsigned long *term, unsigned long *request_seqno, int sync);
int raft_get(RAFT_DATA_TYPE *data, unsigned long index, unsigned long term);
int raft_index_from_put(unsigned long *index, unsigned long *term, unsigned long seq_no);
int raft_config_change(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH], unsigned long *index, unsigned long *term);
int raft_observe();
int raft_current_leader(char *woof_name, char *current_leader);

#endif