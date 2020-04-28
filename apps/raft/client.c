#include <time.h>
#include <string.h>
#include "client.h"
#include "raft.h"

int raft_init_client(FILE *fp) {
    if (read_config(fp, &raft_client_members, raft_client_servers) < 0) {
		fprintf(stderr, "can't read config file\n");
		return -1;
	}
    // printf("%d servers:", raft_client_members);
    // int i;
    // for (i = 0; i < raft_client_members; ++i) {
    //     printf(" [%d]%s", i, raft_client_servers[i]);
    // }
    // printf("\n");
    strcpy(raft_client_leader, raft_client_servers[0]);
    raft_client_timeout = 300;
    raft_client_result_delay = 50;
    return 0;
}

void raft_set_client_leader(char *leader) {
    strcpy(raft_client_leader, leader);
}

void raft_set_client_timeout(int timeout) {
    raft_client_timeout = timeout;
}

int raft_put(RAFT_DATA_TYPE *data, unsigned long *index, unsigned long *term, unsigned long *request_seqno, int sync) {
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_REQUEST request;
    memcpy(&request.data, data, sizeof(RAFT_DATA_TYPE)); 
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    unsigned long seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to send put request\n");
        return RAFT_ERROR;
    }
    if (request_seqno != NULL) {
        *request_seqno = seq;
    }
    if (sync) {
        RAFT_CLIENT_PUT_RESULT result;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
        unsigned long latest_result = 0;
        while (latest_result < seq) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                fprintf(stderr, "timeout after %lums\n", raft_client_timeout);
                return RAFT_TIMEOUT;
            }
            usleep(raft_client_result_delay * 1000);
            latest_result = WooFGetLatestSeqno(woof_name);
        }
        printf("successfully put, waiting for commit\n");
        if (WooFGet(woof_name, &result, seq) < 0) {
            fprintf(stderr, "failed to get put result for put request %lu\n", seq);
            return RAFT_ERROR;
        }
        if (result.redirected == 1) {
            printf("not a leader, redirect to %s\n", result.current_leader);
            strcpy(raft_client_leader, result.current_leader);
            return RAFT_REDIRECTED;
        }
        
        RAFT_SERVER_STATE server_state;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
        unsigned long commit_index = 0;
        while (commit_index < result.index) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                fprintf(stderr, "timeout after %lums\n", raft_client_timeout);
                return RAFT_TIMEOUT;
            }
            unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
            if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
                fprintf(stderr, "appended but can't get leader's commit index\n");
                return RAFT_ERROR;
            }
            usleep(raft_client_result_delay * 1000);
            commit_index = server_state.commit_index;
        }

        RAFT_LOG_ENTRY entry;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
        if (WooFGet(woof_name, &entry, result.index) < 0) {
            fprintf(stderr, "appended but can't check committed entry term\n");
            return RAFT_ERROR;
        }
        if (entry.term == result.term) {
            printf("entry %lu committed at term %lu, val: %s, took %lums\n", result.index, entry.term, entry.data.val, get_milliseconds() - begin);
            if (index != NULL) {
                *index = result.index;
            }
            if (term != NULL) {
                *term = entry.term;
            }
            return RAFT_SUCCESS;
        } else {
            printf("entry %lu appended but got overriden at term %lu, val: %s\n", result.index, entry.term, entry.data.val);
            return RAFT_ERROR;
        }
    } else {
        printf("entry appended and is pending, took %lums\n", get_milliseconds() - begin);
        return RAFT_PENDING;
    }
}

int raft_get(RAFT_DATA_TYPE *data, unsigned long index, unsigned long term) {
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_RESULT result;
	char woof_name[RAFT_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
	unsigned long latest_result = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_result)) {
        fprintf(stderr, "failed to get client put result at %lu\n", index);
        return RAFT_ERROR;
    }
	if (latest_result < index) {
		printf("client put request still pending\n");
		return RAFT_PENDING;
	}
	if (WooFGet(woof_name, &result, index) < 0) {
		fprintf(stderr, "can't get put result at %lu\n", index);
        return RAFT_ERROR;
	}
	if (result.redirected == 1) {
		fprintf(stderr, "client put request got redirected\n");
		return RAFT_REDIRECTED;
	} else {
        if (term > 0 && result.term != term) {
            fprintf(stderr, "this request is processed at term %lu not %lu\n", result.term, term);
            return RAFT_ERROR;
        }

        // check if it's committed
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
        unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
        if (WooFInvalid(latest_server_state)) {
            fprintf(stderr, "can't get the latest server state seqno\n");
            return RAFT_ERROR;
        }
        RAFT_SERVER_STATE server_state;
        if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
            fprintf(stderr, "can't get the server state\n");
            return RAFT_ERROR;
        }
        if (server_state.commit_index < result.index) {
            printf("entry not committed yet\n");
            return RAFT_NOT_COMMITTED;
        }

        // check log entry term
        RAFT_LOG_ENTRY entry;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
        if (WooFGet(woof_name, &entry, result.index) < 0) {
            fprintf(stderr, "can't get log entry at %lu\n", result.index);
            return RAFT_ERROR;
        }
        if (term > 0 && entry.term != term) {
            fprintf(stderr, "entry term %lu doesn't match with request term %lu, it got overriden\n", entry.term, term);
            return RAFT_ERROR;
        }
        memcpy(data, &entry.data, sizeof(RAFT_DATA_TYPE));
		printf("request %lu committed an entry at %lu at term %lu: %s\n", index, result.index, result.term, data->val);
		return RAFT_SUCCESS;
	}
}

int raft_index_from_put(unsigned long *index, unsigned long *term, unsigned long seq_no) {
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    while (WooFGetLatestSeqno(woof_name) < seq_no) {
        usleep(raft_client_result_delay * 1000);
    }
    RAFT_CLIENT_PUT_RESULT result;
    if (WooFGet(woof_name, &result, seq_no) < 0) {
        return -1;
    }
    *index = result.index;
    *term = result.term;
    return 0;
}

int raft_config_change(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH], unsigned long *index, unsigned long *term) {
    unsigned long begin = get_milliseconds();

    RAFT_CONFIG_CHANGE_ARG arg;
    arg.observe = 0;
    arg.members = members;
    memcpy(arg.member_woofs, member_woofs, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_WOOF_NAME_LENGTH);
    
    char monitor_name[RAFT_WOOF_NAME_LENGTH];
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(monitor_name, "%s/%s", raft_client_leader, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CONFIG_CHANGE_ARG_WOOF);
    unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_config_change", &arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to send reconfig request\n");
        return RAFT_ERROR;
    }

    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CONFIG_CHANGE_RESULT_WOOF);
    unsigned long latest_reconfig_result = 0;
    while (latest_reconfig_result < seq) {
        if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
            fprintf(stderr, "timeout after %lums\n", get_milliseconds() - begin);
            return RAFT_TIMEOUT;
        }
        usleep(raft_client_result_delay * 1000);
        latest_reconfig_result = WooFGetLatestSeqno(woof_name);
    }
    RAFT_CONFIG_CHANGE_RESULT result;
    if (WooFGet(woof_name, &result, seq) < 0) {
        fprintf(stderr, "failed to get put result for put request %lu\n", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == 1) {
        printf("not a leader, redirect to %s\n", result.current_leader);
        strcpy(raft_client_leader, result.current_leader);
        return RAFT_REDIRECTED;
    }
    if (result.success == 0) {
        printf("reconfig request denied\n");
        return RAFT_ERROR;
    }
    printf("reconfig requested\n");
    return RAFT_SUCCESS;
}

int raft_observe() {
    unsigned long begin = get_milliseconds();

    RAFT_CONFIG_CHANGE_ARG arg;
    arg.observe = 1;
    if (node_woof_name(arg.observer_woof) < 0) {
        fprintf(stderr, "failed to get observer's woof\n");
        return -1;
    }
    
    int i;
    for (i = 0; i < raft_client_members; ++i) {
        char monitor_name[RAFT_WOOF_NAME_LENGTH];
        char woof_name[RAFT_WOOF_NAME_LENGTH];
        sprintf(monitor_name, "%s/%s", raft_client_servers[i], RAFT_MONITOR_NAME);
        sprintf(woof_name, "%s/%s", raft_client_servers[i], RAFT_CONFIG_CHANGE_ARG_WOOF);
        unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_config_change", &arg);
        if (WooFInvalid(seq)) {
            fprintf(stderr, "failed to send observe request\n");
            return RAFT_ERROR;
        }

        sprintf(woof_name, "%s/%s", raft_client_servers[i], RAFT_CONFIG_CHANGE_RESULT_WOOF);
        unsigned long latest_reconfig_result = 0;
        while (latest_reconfig_result < seq) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                fprintf(stderr, "timeout after %lums\n", get_milliseconds() - begin);
                return RAFT_TIMEOUT;
            }
            usleep(raft_client_result_delay * 1000);
            latest_reconfig_result = WooFGetLatestSeqno(woof_name);
        }
        RAFT_CONFIG_CHANGE_RESULT result;
        if (WooFGet(woof_name, &result, seq) < 0) {
            fprintf(stderr, "failed to get put result for observe request %lu\n", seq);
            return RAFT_ERROR;
        }
        if (result.success == 0) {
            printf("observe request denied\n");
            return RAFT_ERROR;
        }
    }
    printf("observe requested\n");
    return RAFT_SUCCESS;
}

int raft_current_leader(char *member_woof, char *current_leader) {
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", member_woof, RAFT_SERVER_STATE_WOOF);
    unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_server_state)) {
        fprintf(stderr, "failed to get the latest seqno from %s\n", woof_name);
        return -1;
    }
    RAFT_SERVER_STATE server_state;
    if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
        fprintf(stderr, "failed to get the latest server_state %lu from %s\n", latest_server_state, woof_name);
        return -1;
    }
    strcpy(current_leader, server_state.current_leader);
    return 0;
}