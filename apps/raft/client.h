#ifndef RAFT_CLIENT_H
#define RAFT_CLIENT_H

#include "raft.h"

#define RAFT_SUCCESS 0
#define RAFT_NOT_COMMITTED 1;
#define RAFT_ERROR -1
#define RAFT_TIMEOUT -2
#define RAFT_REDIRECTED -3

int raft_client_members;
char raft_client_servers[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
char raft_client_leader[RAFT_WOOF_NAME_LENGTH];
int raft_client_timeout;
int raft_client_result_delay;
void raft_init_client(FILE *fp);
void raft_set_client_timeout(int timeout);
int raft_put(RAFT_DATA_TYPE *data, unsigned long *index, unsigned long *term, bool sync);
int raft_get(RAFT_DATA_TYPE *data, unsigned long index, unsigned long term);
int raft_reconfig(int members, char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH], unsigned long *index, unsigned long *term);

#endif