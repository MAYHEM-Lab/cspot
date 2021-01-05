#include "raft_client.h"

#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// return client_put request seqno
unsigned long raft_put(char* raft_leader, RAFT_DATA_TYPE* data, RAFT_CLIENT_PUT_OPTION* opt) {
    RAFT_CLIENT_PUT_REQUEST request = {0};
    request.ts_a = get_milliseconds();
    request.is_handler = 0;
    memcpy(&request.data, data, sizeof(RAFT_DATA_TYPE));
    if (opt != NULL) {
        strcpy(request.callback_woof, opt->callback_woof);
        strcpy(request.callback_handler, opt->callback_handler);
        memcpy(request.extra_woof, opt->extra_woof, sizeof(request.extra_woof));
        request.extra_seqno = opt->extra_seqno;
    }
    char woof_name[RAFT_NAME_LENGTH] = {0};
    if (raft_leader == NULL) {
        sprintf(woof_name, "%s", RAFT_CLIENT_PUT_REQUEST_WOOF);
    } else {
        sprintf(woof_name, "%s/%s", raft_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    }
    unsigned long seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_error_msg, "failed to send client_put request");
        return RAFT_ERROR;
    }
    return seq;
}

int raft_check_committed(char* raft_leader, uint64_t index) {
    char woof_name[RAFT_NAME_LENGTH] = {0};
    // check if it's committed
    if (raft_leader == NULL) {
        sprintf(woof_name, "%s", RAFT_SERVER_STATE_WOOF);
    } else {
        sprintf(woof_name, "%s/%s", raft_leader, RAFT_SERVER_STATE_WOOF);
    }
    RAFT_SERVER_STATE server_state = {0};
    int err = WooFGet(woof_name, &server_state, 0);
    if (err < 0) {
        sprintf(raft_error_msg, "can't get the server state");
        return RAFT_ERROR;
    }
    if (server_state.commit_index < index) {
        sprintf(
            raft_error_msg, "entry not committed yet: index %lu, commit_index %lu", index, server_state.commit_index);
        return RAFT_NOT_COMMITTED;
    }
    return 0;
}

int raft_get(char* raft_leader, RAFT_DATA_TYPE* data, uint64_t index) {
    char woof_name[RAFT_NAME_LENGTH] = {0};
    RAFT_LOG_ENTRY entry = {0};
    if (raft_leader == NULL) {
        sprintf(woof_name, "%s", RAFT_LOG_ENTRIES_WOOF);
    } else {
        sprintf(woof_name, "%s/%s", raft_leader, RAFT_LOG_ENTRIES_WOOF);
    }
    if (WooFGet(woof_name, &entry, (unsigned long)index) < 0) {
        sprintf(raft_error_msg, "can't get log entry at %lu", (unsigned long)index);
        return RAFT_ERROR;
    }
    memcpy(data, &entry.data, sizeof(RAFT_DATA_TYPE));
    return 0;
}

uint64_t
raft_put_handler(char* raft_leader, char* handler, void* data, unsigned long size, RAFT_CLIENT_PUT_OPTION* opt) {
    if (size > sizeof(RAFT_LOG_HANDLER_ENTRY) - RAFT_NAME_LENGTH) {
        sprintf(raft_error_msg,
                "size %lu is greater than the maximum a log entry can support(%lu)",
                size,
                sizeof(RAFT_LOG_HANDLER_ENTRY) - RAFT_NAME_LENGTH);
        return -1;
    }
    uint64_t begin = get_milliseconds();
    RAFT_CLIENT_PUT_REQUEST request = {0};
    request.ts_a = get_milliseconds();
    request.is_handler = 1;
    if (opt != NULL) {
        strcpy(request.callback_woof, opt->callback_woof);
        strcpy(request.callback_handler, opt->callback_handler);
        memcpy(request.extra_woof, opt->extra_woof, sizeof(request.extra_woof));
        request.extra_seqno = opt->extra_seqno;
    }
    RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)&request.data;
    memset(handler_entry, 0, sizeof(RAFT_LOG_HANDLER_ENTRY));
    strcpy(handler_entry->handler, handler);
    memcpy(handler_entry->ptr, data, size);
    char woof_name[RAFT_NAME_LENGTH] = {0};
    if (raft_leader == NULL) {
        sprintf(woof_name, "%s", RAFT_CLIENT_PUT_REQUEST_WOOF);
    } else {
        sprintf(woof_name, "%s/%s", raft_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
    }
    unsigned long seq = WooFPut(woof_name, NULL, &request);
    if (WooFInvalid(seq)) {
        sprintf(raft_error_msg, "failed to send put request\n");
        return RAFT_ERROR;
    }
    return seq;
}

int raft_client_put_result(char* raft_leader, RAFT_CLIENT_PUT_RESULT* result, unsigned long seq_no) {
    char woof_name[RAFT_NAME_LENGTH] = {0};
    if (raft_leader == NULL) {
        sprintf(woof_name, "%s", RAFT_CLIENT_PUT_RESULT_WOOF);
    } else {
        sprintf(woof_name, "%s/%s", raft_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    }
    if (WooFGetLatestSeqno(woof_name) < seq_no) {
        sprintf(raft_error_msg, "client_put result not appended yet");
        return RAFT_WAITING_RESULT;
    }
    if (WooFGet(woof_name, result, seq_no) < 0) {
        sprintf(raft_error_msg, "faield to get client_put result at %lu from %s", seq_no, woof_name);
        return RAFT_ERROR;
    }
    return 0;
}

int raft_is_error(uint64_t code) {
    return code >= RAFT_OVERRIDEN && code <= RAFT_ERROR;
}

int raft_is_leader() {
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        sprintf(raft_error_msg, "failed to get the current server_state");
        return -1;
    }
    return server_state.role == RAFT_LEADER;
}