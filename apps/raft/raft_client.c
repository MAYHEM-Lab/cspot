#include "raft_client.h"

#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"

#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int raft_init_client(int members, char replicas[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
    raft_client_members = members;
    memcpy(raft_client_replicas, replicas, sizeof(raft_client_replicas));
    strcpy(raft_client_leader, raft_client_replicas[0]);
    raft_client_result_delay = DHT_CLIENT_DEFAULT_RESULT_DELAY;
    return 0;
}

char* raft_get_client_leader() {
    return raft_client_leader;
}

void raft_set_client_leader(char* leader) {
    strcpy(raft_client_leader, leader);
}

void raft_set_client_result_delay(int delay) {
    raft_client_result_delay = delay;
}

// return log index
uint64_t raft_sync_put(RAFT_DATA_TYPE* data, int timeout) {
    uint64_t begin = get_milliseconds();

    RAFT_CLIENT_PUT_REQUEST request = {0};
    request.is_handler = 0;
    request.created_ts = get_milliseconds();
    memcpy(&request.data, data, sizeof(RAFT_DATA_TYPE));
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    unsigned long seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_error_msg, "failed to send put request\n");
        return RAFT_ERROR;
    }
    RAFT_CLIENT_PUT_RESULT result = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    unsigned long latest_result = 0;
    while (latest_result < seq) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_error_msg, "timeout after %dms\n", timeout);
            return RAFT_TIMEOUT;
        }
        usleep(raft_client_result_delay * 1000);
        latest_result = WooFGetLatestSeqno(woof_name);
    }
    if (WooFGet(woof_name, &result, seq) < 0) {
        sprintf(raft_error_msg, "failed to get put result for put request %lu\n", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == 1) {
        sprintf(raft_error_msg, "not a leader, redirect to %s\n", result.current_leader);
        strcpy(raft_client_leader, result.current_leader);
        return RAFT_REDIRECTED;
    }

    RAFT_SERVER_STATE server_state = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
    uint64_t commit_index = 0;
    while (commit_index < result.index) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_error_msg, "timeout after %dms\n", timeout);
            return RAFT_TIMEOUT;
        }
        unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
        if (WooFInvalid(latest_server_state)) {
            sprintf(raft_error_msg, "failed to get latest server state\n");
            return RAFT_ERROR;
        }
        if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
            sprintf(raft_error_msg, "appended but can't get leader's commit index\n");
            return RAFT_ERROR;
        }
        usleep(raft_client_result_delay * 1000);
        commit_index = server_state.commit_index;
    }

    RAFT_LOG_ENTRY entry = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
    if (WooFGet(woof_name, &entry, result.index) < 0) {
        sprintf(raft_error_msg, "appended but can't check committed entry term\n");
        return RAFT_ERROR;
    }
    if (entry.term == result.term) {
#ifdef DEBUG
        printf("entry %" PRIu64 " committed at term %" PRIu64 ", val: %s, took %" PRIu64 "ms\n",
               result.index,
               entry.term,
               entry.data.val,
               get_milliseconds() - begin);
#endif
        return result.index;
    } else {
        sprintf(raft_error_msg,
                "entry %" PRIu64 " appended but got overriden at term %" PRIu64 ", val: %s\n",
                result.index,
                entry.term,
                entry.data.val);
        return RAFT_ERROR;
    }
}

// return client_put request seqno
unsigned long raft_async_put(RAFT_DATA_TYPE* data) {
    uint64_t begin = get_milliseconds();

    RAFT_CLIENT_PUT_REQUEST request = {0};
    request.is_handler = 0;
    request.created_ts = get_milliseconds();
    memcpy(&request.data, data, sizeof(RAFT_DATA_TYPE));
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    unsigned seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_error_msg, "failed to send client_put request");
        return RAFT_ERROR;
    }
    return seq;
}

// return term or ERROR
uint64_t raft_sync_get(RAFT_DATA_TYPE* data, uint64_t index, int committed_only) {
    char woof_name[RAFT_NAME_LENGTH] = {0};
    // check if it's committed
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
    unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_server_state)) {
        sprintf(raft_error_msg, "can't get the latest server state seqno");
        return RAFT_ERROR;
    }
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
        sprintf(raft_error_msg, "can't get the server state");
        return RAFT_ERROR;
    }
    if (committed_only && server_state.commit_index < index) {
        sprintf(raft_error_msg, "entry not committed yet");
        return RAFT_NOT_COMMITTED;
    }

    // check log entry term
    RAFT_LOG_ENTRY entry = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
    if (WooFGet(woof_name, &entry, (unsigned long)index) < 0) {
        sprintf(raft_error_msg, "can't get log entry at %lu", (unsigned long)index);
        return RAFT_ERROR;
    }
    memcpy(data, &entry.data, sizeof(RAFT_DATA_TYPE));
    return entry.term;
}

// return term or ERROR
uint64_t raft_async_get(RAFT_DATA_TYPE* data, unsigned long client_put_seqno) {
    uint64_t begin = get_milliseconds();

    uint64_t index, term;
    if (raft_client_put_result(&index, &term, client_put_seqno) < 0) {
        sprintf(raft_error_msg, "can't get index and term from client_put request");
        return RAFT_ERROR;
    }

    uint64_t committed_term = raft_sync_get(data, index, 1);
    if (raft_is_error(committed_term)) {
        return committed_term;
    }
    if (committed_term != term) {
        sprintf(raft_error_msg, "client_request got overriden by later term");
        return RAFT_OVERRIDEN;
    }
    return committed_term;
}

uint64_t raft_put_handler(char* handler, void* data, unsigned long size, int monitored, int timeout) {
    if (size > sizeof(RAFT_LOG_HANDLER_ENTRY) - RAFT_NAME_LENGTH) {
        sprintf(raft_error_msg,
                "size %lu is greater than the maximum a log entry can support(%lu)",
                size,
                sizeof(RAFT_LOG_HANDLER_ENTRY) - RAFT_NAME_LENGTH);
        return -1;
    }
    uint64_t begin = get_milliseconds();
    RAFT_CLIENT_PUT_REQUEST request = {0};
    request.is_handler = 1;
    request.created_ts = get_milliseconds();
    RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)&request.data;
    memset(handler_entry, 0, sizeof(RAFT_LOG_HANDLER_ENTRY));
    strcpy(handler_entry->handler, handler);
    memcpy(handler_entry->ptr, data, size);
    handler_entry->monitored = monitored;
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    unsigned long seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_error_msg, "failed to send put request\n");
        return RAFT_ERROR;
    }
    RAFT_CLIENT_PUT_RESULT result = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    unsigned long latest_result = 0;
    while (latest_result < seq) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_error_msg, "timeout after %dms\n", timeout);
            return RAFT_TIMEOUT;
        }
        usleep(raft_client_result_delay * 1000);
        latest_result = WooFGetLatestSeqno(woof_name);
    }
    if (WooFGet(woof_name, &result, seq) < 0) {
        sprintf(raft_error_msg, "failed to get put result for put request %lu\n", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == 1) {
        sprintf(raft_error_msg, "not a leader, redirect to %s\n", result.current_leader);
        strcpy(raft_client_leader, result.current_leader);
        return RAFT_REDIRECTED;
    }

    RAFT_SERVER_STATE server_state = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
    uint64_t commit_index = 0;
    while (commit_index < result.index) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_error_msg, "timeout after %dms\n", timeout);
            return RAFT_TIMEOUT;
        }
        unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
        if (WooFInvalid(latest_server_state)) {
            sprintf(raft_error_msg, "can't get the latest server state seqno");
            return RAFT_ERROR;
        }
        if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
            sprintf(raft_error_msg, "appended but can't get leader's commit index\n");
            return RAFT_ERROR;
        }
        usleep(raft_client_result_delay * 1000);
        commit_index = server_state.commit_index;
    }

    RAFT_LOG_ENTRY entry = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
    if (WooFGet(woof_name, &entry, result.index) < 0) {
        sprintf(raft_error_msg, "appended but can't check committed entry term\n");
        return RAFT_ERROR;
    }
    if (entry.term == result.term) {
        return result.index;
    } else {
        sprintf(raft_error_msg,
                "entry %" PRIu64 " appended but got overriden at term %" PRIu64 ", val: %s\n",
                result.index,
                entry.term,
                entry.data.val);
        return RAFT_ERROR;
    }
}

uint64_t raft_sessionless_put_handler(
    char* raft_leader, char* handler, void* data, unsigned long size, int monitored, int timeout) {
    if (size > sizeof(RAFT_LOG_HANDLER_ENTRY) - RAFT_NAME_LENGTH) {
        sprintf(raft_error_msg,
                "size %lu is greater than the maximum a log entry can support(%lu)",
                size,
                sizeof(RAFT_LOG_HANDLER_ENTRY) - RAFT_NAME_LENGTH);
        return -1;
    }
    char leader[RAFT_NAME_LENGTH] = {0};
    strcpy(leader, raft_leader);
    char woof_name[RAFT_NAME_LENGTH] = {0};

    uint64_t begin = get_milliseconds();
    while (1) {
        RAFT_CLIENT_PUT_REQUEST request = {0};
        request.is_handler = 1;
        request.created_ts = get_milliseconds();
        RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)&request.data;
        memset(handler_entry, 0, sizeof(RAFT_LOG_HANDLER_ENTRY));
        strcpy(handler_entry->handler, handler);
        memcpy(handler_entry->ptr, data, size);
        handler_entry->monitored = monitored;
        sprintf(woof_name, "%s/%s", leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
        unsigned long seq = WooFPut(woof_name, NULL, &request);

        if (WooFInvalid(seq)) {
            sprintf(raft_error_msg, "failed to send put request\n");
            return RAFT_ERROR;
        }
        RAFT_CLIENT_PUT_RESULT result = {0};
        sprintf(woof_name, "%s/%s", leader, RAFT_CLIENT_PUT_RESULT_WOOF);
        unsigned long latest_result = 0;
        while (latest_result < seq) {
            if (timeout > 0 && get_milliseconds() - begin > timeout) {
                sprintf(raft_error_msg, "timeout after %dms\n", timeout);
                return RAFT_TIMEOUT;
            }
            usleep(raft_client_result_delay * 1000);
            latest_result = WooFGetLatestSeqno(woof_name);
        }
        if (WooFGet(woof_name, &result, seq) < 0) {
            sprintf(raft_error_msg, "failed to get put result for put request %lu\n", seq);
            return RAFT_ERROR;
        }
        if (result.redirected == 1) {
            sprintf(raft_error_msg, "not a leader, redirect to %s\n", result.current_leader);
            strcpy(leader, result.current_leader);
            continue;
        }

        RAFT_SERVER_STATE server_state = {0};
        sprintf(woof_name, "%s/%s", leader, RAFT_SERVER_STATE_WOOF);
        uint64_t commit_index = 0;
        while (commit_index < result.index) {
            if (timeout > 0 && get_milliseconds() - begin > timeout) {
                sprintf(raft_error_msg, "timeout after %dms\n", timeout);
                return RAFT_TIMEOUT;
            }
            unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
            if (WooFInvalid(latest_server_state)) {
                sprintf(raft_error_msg, "can't get the latest server state seqno");
                return RAFT_ERROR;
            }
            if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
                sprintf(raft_error_msg, "appended but can't get leader's commit index\n");
                return RAFT_ERROR;
            }
            usleep(raft_client_result_delay * 1000);
            commit_index = server_state.commit_index;
        }
        RAFT_LOG_ENTRY entry = {0};
        sprintf(woof_name, "%s/%s", leader, RAFT_LOG_ENTRIES_WOOF);
        if (WooFGet(woof_name, &entry, result.index) < 0) {
            sprintf(raft_error_msg, "appended but can't check committed entry term\n");
            return RAFT_ERROR;
        }
        if (entry.term == result.term) {
            return result.index;
        } else {
            sprintf(raft_error_msg,
                    "entry %" PRIu64 " appended but got overriden at term %" PRIu64 ", val: %s\n",
                    result.index,
                    entry.term,
                    entry.data.val);
            return RAFT_ERROR;
        }
    }
}

int raft_client_put_result(uint64_t* index, uint64_t* term, unsigned long seq_no) {
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    if (WooFGetLatestSeqno(woof_name) < seq_no) {
        return -1;
    }
    RAFT_CLIENT_PUT_RESULT result = {0};
    if (WooFGet(woof_name, &result, seq_no) < 0) {
        return -1;
    }
    *index = result.index;
    *term = result.term;
    return 0;
}

int raft_config_change(int members,
                       char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                       int timeout) {
    uint64_t begin = get_milliseconds();

    RAFT_CONFIG_CHANGE_ARG arg = {0};
    arg.observe = 0;
    arg.members = members;
    memcpy(arg.member_woofs, member_woofs, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_NAME_LENGTH);

    char monitor_name[RAFT_NAME_LENGTH] = {0};
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(monitor_name, "%s/%s", raft_client_leader, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CONFIG_CHANGE_ARG_WOOF);
    unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_config_change", &arg, 0);
    if (WooFInvalid(seq)) {
        sprintf(raft_error_msg, "failed to send reconfig request");
        return RAFT_ERROR;
    }

    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CONFIG_CHANGE_RESULT_WOOF);
    unsigned long latest_reconfig_result = 0;
    while (latest_reconfig_result < seq) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(raft_error_msg, "timeout after %" PRIu64 "ms", get_milliseconds() - begin);
            return RAFT_TIMEOUT;
        }
        usleep(raft_client_result_delay * 1000);
        latest_reconfig_result = WooFGetLatestSeqno(woof_name);
    }
    RAFT_CONFIG_CHANGE_RESULT result = {0};
    if (WooFGet(woof_name, &result, seq) < 0) {
        sprintf(raft_error_msg, "failed to get put result for put request %lu", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == 1) {
        sprintf(raft_error_msg, "not a leader, redirect to %s", result.current_leader);
        strcpy(raft_client_leader, result.current_leader);
        return RAFT_REDIRECTED;
    }
    if (result.success == 0) {
        sprintf(raft_error_msg, "reconfig request denied");
        return RAFT_ERROR;
    }
    printf("reconfig requested\n");
    return RAFT_SUCCESS;
}

int raft_observe(char oberver_woof_name[RAFT_NAME_LENGTH], int timeout) {
    uint64_t begin = get_milliseconds();

    RAFT_CONFIG_CHANGE_ARG arg = {0};
    arg.observe = 1;
    memcpy(arg.observer_woof, oberver_woof_name, sizeof(arg.observer_woof));

    int i;
    for (i = 0; i < raft_client_members; ++i) {
        char monitor_name[RAFT_NAME_LENGTH] = {0};
        char woof_name[RAFT_NAME_LENGTH] = {0};
        sprintf(monitor_name, "%s/%s", raft_client_replicas[i], RAFT_MONITOR_NAME);
        sprintf(woof_name, "%s/%s", raft_client_replicas[i], RAFT_CONFIG_CHANGE_ARG_WOOF);
        unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_config_change", &arg, 0);
        if (WooFInvalid(seq)) {
            sprintf(raft_error_msg, "failed to send observe request");
            return RAFT_ERROR;
        }

        sprintf(woof_name, "%s/%s", raft_client_replicas[i], RAFT_CONFIG_CHANGE_RESULT_WOOF);
        unsigned long latest_reconfig_result = 0;
        while (latest_reconfig_result < seq) {
            if (timeout > 0 && get_milliseconds() - begin > timeout) {
                sprintf(raft_error_msg, "timeout after %" PRIu64 "ms", get_milliseconds() - begin);
                return RAFT_TIMEOUT;
            }
            usleep(raft_client_result_delay * 1000);
            latest_reconfig_result = WooFGetLatestSeqno(woof_name);
        }
        RAFT_CONFIG_CHANGE_RESULT result = {0};
        if (WooFGet(woof_name, &result, seq) < 0) {
            sprintf(raft_error_msg, "failed to get put result for observe request %lu", seq);
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

int raft_current_leader(char* member_woof, char* current_leader) {
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(woof_name, "%s/%s", member_woof, RAFT_SERVER_STATE_WOOF);
    unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_server_state)) {
        sprintf(raft_error_msg, "failed to get the latest seqno from %s", woof_name);
        return -1;
    }
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
        sprintf(raft_error_msg, "failed to get the latest server_state %lu from %s", latest_server_state, woof_name);
        return -1;
    }
    strcpy(current_leader, server_state.current_leader);
    return 0;
}

int raft_is_error(uint64_t code) {
    return code >= RAFT_OVERRIDEN && code <= RAFT_ERROR;
}

int raft_is_leader() {
    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        sprintf(raft_error_msg, "failed to get the current server_state");
        return -1;
    }
    return server_state.role == RAFT_LEADER;
}