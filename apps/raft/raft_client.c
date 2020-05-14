#include <time.h>
#include <string.h>
#include "raft_client.h"
#include "raft.h"

int raft_init_client(FILE *config) {
    if (read_config(config, &raft_client_members, raft_client_servers) < 0) {
		sprintf(raft_client_error_msg, "can't read config file\n");
		return -1;
	}
    strcpy(raft_client_leader, raft_client_servers[0]);
    raft_client_result_delay = 50;
    return 0;
}

void raft_set_client_leader(char *leader) {
    strcpy(raft_client_leader, leader);
}

// return log index
unsigned long raft_sync_put(RAFT_DATA_TYPE *data, int timeout) {
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_REQUEST request;
    request.created_ts = get_milliseconds();
    memcpy(&request.data, data, sizeof(RAFT_DATA_TYPE)); 
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    unsigned long seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_client_error_msg, "failed to send put request\n");
        return RAFT_ERROR;
    }
    RAFT_CLIENT_PUT_RESULT result;
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    unsigned long latest_result = 0;
    while (latest_result < seq) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_client_error_msg, "timeout after %lums\n", timeout);
            return RAFT_TIMEOUT;
        }
        usleep(raft_client_result_delay * 1000);
        latest_result = WooFGetLatestSeqno(woof_name);
    }
    if (WooFGet(woof_name, &result, seq) < 0) {
        sprintf(raft_client_error_msg, "failed to get put result for put request %lu\n", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == 1) {
        sprintf(raft_client_error_msg, "not a leader, redirect to %s\n", result.current_leader);
        strcpy(raft_client_leader, result.current_leader);
        return RAFT_REDIRECTED;
    }
    
    RAFT_SERVER_STATE server_state;
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
    unsigned long commit_index = 0;
    while (commit_index < result.index) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_client_error_msg, "timeout after %lums\n", timeout);
            return RAFT_TIMEOUT;
        }
        unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
        if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
            sprintf(raft_client_error_msg, "appended but can't get leader's commit index\n");
            return RAFT_ERROR;
        }
        usleep(raft_client_result_delay * 1000);
        commit_index = server_state.commit_index;
    }

    RAFT_LOG_ENTRY entry;
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
    if (WooFGet(woof_name, &entry, result.index) < 0) {
        sprintf(raft_client_error_msg, "appended but can't check committed entry term\n");
        return RAFT_ERROR;
    }
    if (entry.term == result.term) {
        printf("entry %lu committed at term %lu, val: %s, took %lums\n", result.index, entry.term, entry.data.val, get_milliseconds() - begin);
        return result.index;
    } else {
        sprintf(raft_client_error_msg, "entry %lu appended but got overriden at term %lu, val: %s\n", result.index, entry.term, entry.data.val);
        return RAFT_ERROR;
    }
}

// return client_put request seqno
unsigned long raft_async_put(RAFT_DATA_TYPE *data) {
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_REQUEST request;
    request.created_ts = get_milliseconds();
    memcpy(&request.data, data, sizeof(RAFT_DATA_TYPE)); 
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    unsigned seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_client_error_msg, "failed to send client_put request");
        return RAFT_ERROR;
    }
    return seq;
}

// return term or ERROR
unsigned long raft_sync_get(RAFT_DATA_TYPE *data, unsigned long index) {
    char woof_name[RAFT_NAME_LENGTH];
    // check if it's committed
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
    unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_server_state)) {
        sprintf(raft_client_error_msg, "can't get the latest server state seqno");
        return RAFT_ERROR;
    }
    RAFT_SERVER_STATE server_state;
    if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
        sprintf(raft_client_error_msg, "can't get the server state");
        return RAFT_ERROR;
    }
    if (server_state.commit_index < index) {
        sprintf(raft_client_error_msg, "entry not committed yet");
        return RAFT_NOT_COMMITTED;
    }

    // check log entry term
    RAFT_LOG_ENTRY entry;
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
    if (WooFGet(woof_name, &entry, index) < 0) {
        sprintf(raft_client_error_msg, "can't get log entry at %lu", index);
        return RAFT_ERROR;
    }
    memcpy(data, &entry.data, sizeof(RAFT_DATA_TYPE));
    printf("entry %lu is committed at term %lu\n", index, entry.term);
    return entry.term;
}

// return term or ERROR
unsigned long raft_async_get(RAFT_DATA_TYPE *data, unsigned long client_put_seqno) {
    unsigned long begin = get_milliseconds();

    unsigned long index, term;
    if (raft_client_put_result(&index, &term, client_put_seqno) < 0) {
        sprintf(raft_client_error_msg, "can't get index and term from client_put request");
        return RAFT_ERROR;
    }

	unsigned long committed_term = raft_sync_get(data, index);
    if (raft_is_error(committed_term)) {
        return committed_term;
    }
    if (committed_term != term) {
        sprintf(raft_client_error_msg, "client_request got overriden by later term");
        return RAFT_OVERRIDEN;
    }
    return committed_term;
}

int raft_client_put_result(unsigned long *index, unsigned long *term, unsigned long seq_no) {
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    if (WooFGetLatestSeqno(woof_name) < seq_no) {
        return -1;
    }
    RAFT_CLIENT_PUT_RESULT result;
    if (WooFGet(woof_name, &result, seq_no) < 0) {
        return -1;
    }
    *index = result.index;
    *term = result.term;
    return 0;
}

int raft_config_change(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH], int timeout) {
    unsigned long begin = get_milliseconds();

    RAFT_CONFIG_CHANGE_ARG arg;
    arg.observe = 0;
    arg.members = members;
    memcpy(arg.member_woofs, member_woofs, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_NAME_LENGTH);
    
    char monitor_name[RAFT_NAME_LENGTH];
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(monitor_name, "%s/%s", raft_client_leader, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CONFIG_CHANGE_ARG_WOOF);
    unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_config_change", &arg);
    if (WooFInvalid(seq)) {
        sprintf(raft_client_error_msg, "failed to send reconfig request");
        return RAFT_ERROR;
    }

    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CONFIG_CHANGE_RESULT_WOOF);
    unsigned long latest_reconfig_result = 0;
    while (latest_reconfig_result < seq) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_client_error_msg, "timeout after %lums", get_milliseconds() - begin);
            return RAFT_TIMEOUT;
        }
        usleep(raft_client_result_delay * 1000);
        latest_reconfig_result = WooFGetLatestSeqno(woof_name);
    }
    RAFT_CONFIG_CHANGE_RESULT result;
    if (WooFGet(woof_name, &result, seq) < 0) {
        sprintf(raft_client_error_msg, "failed to get put result for put request %lu", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == 1) {
        sprintf(raft_client_error_msg, "not a leader, redirect to %s", result.current_leader);
        strcpy(raft_client_leader, result.current_leader);
        return RAFT_REDIRECTED;
    }
    if (result.success == 0) {
        sprintf(raft_client_error_msg, "reconfig request denied");
        return RAFT_ERROR;
    }
    printf("reconfig requested\n");
    return RAFT_SUCCESS;
}

int raft_observe(int timeout) {
    unsigned long begin = get_milliseconds();

    RAFT_CONFIG_CHANGE_ARG arg;
    arg.observe = 1;
    if (node_woof_name(arg.observer_woof) < 0) {
        sprintf(raft_client_error_msg, "failed to get observer's woof");
        return -1;
    }
    
    int i;
    for (i = 0; i < raft_client_members; ++i) {
        char monitor_name[RAFT_NAME_LENGTH];
        char woof_name[RAFT_NAME_LENGTH];
        sprintf(monitor_name, "%s/%s", raft_client_servers[i], RAFT_MONITOR_NAME);
        sprintf(woof_name, "%s/%s", raft_client_servers[i], RAFT_CONFIG_CHANGE_ARG_WOOF);
        unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_config_change", &arg);
        if (WooFInvalid(seq)) {
            sprintf(raft_client_error_msg, "failed to send observe request");
            return RAFT_ERROR;
        }

        sprintf(woof_name, "%s/%s", raft_client_servers[i], RAFT_CONFIG_CHANGE_RESULT_WOOF);
        unsigned long latest_reconfig_result = 0;
        while (latest_reconfig_result < seq) {
            if (timeout > 0 && get_milliseconds() - begin > timeout) {
                sprintf(raft_client_error_msg, "timeout after %lums", get_milliseconds() - begin);
                return RAFT_TIMEOUT;
            }
            usleep(raft_client_result_delay * 1000);
            latest_reconfig_result = WooFGetLatestSeqno(woof_name);
        }
        RAFT_CONFIG_CHANGE_RESULT result;
        if (WooFGet(woof_name, &result, seq) < 0) {
            sprintf(raft_client_error_msg, "failed to get put result for observe request %lu", seq);
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
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", member_woof, RAFT_SERVER_STATE_WOOF);
    unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_server_state)) {
        sprintf(raft_client_error_msg, "failed to get the latest seqno from %s", woof_name);
        return -1;
    }
    RAFT_SERVER_STATE server_state;
    if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
        sprintf(raft_client_error_msg, "failed to get the latest server_state %lu from %s", latest_server_state, woof_name);
        return -1;
    }
    strcpy(current_leader, server_state.current_leader);
    return 0;
}

int raft_is_error(unsigned long code) {
    return code >= RAFT_OVERRIDEN && code <= RAFT_ERROR;
}