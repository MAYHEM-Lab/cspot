#include "raft.h"

#include "monitor.h"
#include "raft_utils.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char RAFT_WOOF_TO_CREATE[][RAFT_NAME_LENGTH] = {
    RAFT_DEBUG_INTERRUPT_WOOF,     RAFT_LOG_ENTRIES_WOOF,           RAFT_SERVER_STATE_WOOF,
    RAFT_HEARTBEAT_WOOF,           RAFT_TIMEOUT_CHECKER_WOOF,       RAFT_APPEND_ENTRIES_ROUTINE_WOOF,
    RAFT_APPEND_ENTRIES_ARG_WOOF,  RAFT_APPEND_ENTRIES_RESULT_WOOF, RAFT_LOG_HANDLER_ENTRIES_WOOF,
    RAFT_CLIENT_PUT_REQUEST_WOOF,  RAFT_CLIENT_PUT_ARG_WOOF,        RAFT_CLIENT_PUT_RESULT_WOOF,
    RAFT_CONFIG_CHANGE_ARG_WOOF,   RAFT_CONFIG_CHANGE_RESULT_WOOF,  RAFT_CHECK_APPEND_RESULT_WOOF,
    RAFT_UPDATE_COMMIT_INDEX_WOOF, RAFT_INVOKE_COMMITTED_WOOF,      RAFT_REPLICATE_ENTRIES_WOOF,
    RAFT_REQUEST_VOTE_ARG_WOOF,    RAFT_REQUEST_VOTE_RESULT_WOOF};

unsigned long RAFT_WOOF_ELEMENT_SIZE[] = {sizeof(RAFT_DEBUG_INTERRUPT_ARG),
                                          sizeof(RAFT_LOG_ENTRY),
                                          sizeof(RAFT_SERVER_STATE),
                                          sizeof(RAFT_HEARTBEAT),
                                          sizeof(RAFT_TIMEOUT_CHECKER_ARG),
                                          sizeof(RAFT_APPEND_ENTRIES_ROUTINE_ARG),
                                          sizeof(RAFT_APPEND_ENTRIES_ARG),
                                          sizeof(RAFT_APPEND_ENTRIES_RESULT),
                                          sizeof(RAFT_LOG_HANDLER_ENTRY),
                                          sizeof(RAFT_CLIENT_PUT_REQUEST),
                                          sizeof(RAFT_CLIENT_PUT_ARG),
                                          sizeof(RAFT_CLIENT_PUT_RESULT),
                                          sizeof(RAFT_CONFIG_CHANGE_ARG),
                                          sizeof(RAFT_CONFIG_CHANGE_RESULT),
                                          sizeof(RAFT_CHECK_APPEND_RESULT_ARG),
                                          sizeof(RAFT_UPDATE_COMMIT_INDEX_ARG),
                                          sizeof(RAFT_INVOKE_COMMITTED_ARG),
                                          sizeof(RAFT_REPLICATE_ENTRIES_ARG),
                                          sizeof(RAFT_REQUEST_VOTE_ARG),
                                          sizeof(RAFT_REQUEST_VOTE_RESULT)};

unsigned long RAFT_WOOF_HISTORY_SIZE[] = {
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_DEBUG_INTERRUPT_ARG
    RAFT_WOOF_HISTORY_SIZE_LONG,  // RAFT_LOG_ENTRY
    RAFT_WOOF_HISTORY_SIZE_LONG,  // RAFT_SERVER_STATE
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_HEARTBEAT
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_TIMEOUT_CHECKER_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_APPEND_ENTRIES_ROUTINE_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_APPEND_ENTRIES_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_APPEND_ENTRIES_RESULT
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_LOG_HANDLER_ENTRY
    RAFT_WOOF_HISTORY_SIZE_LONG,  // RAFT_CLIENT_PUT_REQUEST
    RAFT_WOOF_HISTORY_SIZE_LONG,  // RAFT_CLIENT_PUT_ARG
    RAFT_WOOF_HISTORY_SIZE_LONG,  // RAFT_CLIENT_PUT_RESULT
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_CONFIG_CHANGE_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_CONFIG_CHANGE_RESULT
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_CHECK_APPEND_RESULT_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_UPDATE_COMMIT_INDEX
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_INVOKE_COMMITTED_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_REPLICATE_ENTRIES_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT, // RAFT_REQUEST_VOTE_ARG
    RAFT_WOOF_HISTORY_SIZE_SHORT  // RAFT_REQUEST_VOTE_RESULT
};

int get_server_state(RAFT_SERVER_STATE* server_state) {
    unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
    if (WooFInvalid(last_server_state)) {
        sprintf(raft_error_msg, "failed to get the latest seqno of server_state");
        return -1;
    }
    if (last_server_state == 0) {
        sprintf(raft_error_msg, "server_state is not initialized yet, latest seq_no is 0");
        return -1;
    }
    if (WooFGet(RAFT_SERVER_STATE_WOOF, server_state, last_server_state) < 0) {
        sprintf(raft_error_msg, "failed to get the latest server_state");
        return -1;
    }
    return 0;
}

uint64_t random_timeout(unsigned long seed, int min, int max) {
    srand(seed);
    return (uint64_t)(min + (rand() % (max - min)));
}

int32_t member_id(char* woof_name, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
    int32_t i;
    for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
        if (strcmp(woof_name, member_woofs[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int encode_config(char* dst, int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
    sprintf(dst, "%d;", members);
    int i;
    for (i = 0; i < members; ++i) {
        sprintf(dst + strlen(dst), "%s;", member_woofs[i]);
    }
    return strlen(dst);
}

int decode_config(char* src, int* members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
    int len = 0;
    char* token;
    token = strtok(src, ";");
    len += strlen(token) + 1;
    *members = (int)strtol(token, (char**)NULL, 10);
    if (*members == 0) {
        return -1;
    }
    memset(member_woofs, 0, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_NAME_LENGTH);
    int i;
    for (i = 0; i < *members; ++i) {
        token = strtok(NULL, ";");
        strcpy(member_woofs[i], token);
        len += strlen(token) + 1;
    }
    return len;
}

int qsort_strcmp(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}

int compute_joint_config(int old_members,
                         char old_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                         int new_members,
                         char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                         int* joint_members,
                         char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
    if (old_members + new_members > RAFT_MAX_MEMBERS) {
        return -1;
    }
    memset(joint_member_woofs, 0, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_NAME_LENGTH);
    int i;
    for (i = 0; i < old_members; ++i) {
        strcpy(joint_member_woofs[i], old_member_woofs[i]);
    }
    for (i = 0; i < new_members; ++i) {
        strcpy(joint_member_woofs[old_members + i], new_member_woofs[i]);
    }
    qsort(joint_member_woofs, old_members + new_members, RAFT_NAME_LENGTH, qsort_strcmp);
    int tail = 0;
    for (i = 0; i < old_members + new_members; ++i) {
        if (tail > 0 && strcmp(joint_member_woofs[i], joint_member_woofs[tail - 1]) == 0) {
            continue;
        }
        if (tail != i) {
            strcpy(joint_member_woofs[tail], joint_member_woofs[i]);
        }
        ++tail;
    }
    *joint_members = tail;
    return 0;
}

int threads_join(int count, pthread_t* pids) {
    int cnt = 0;
    int i;
    for (i = 0; i < count; ++i) {
        if (pids[i] != 0) {
            int err = pthread_join(pids[i], NULL);
            if (err < 0) {
                return err;
            }
            ++cnt;
        }
    }
    return cnt;
}

int threads_cancel(int count, pthread_t* pids) {
    int cnt = 0;
    int i;
    for (i = 0; i < count; ++i) {
        if (pids[i] != 0) {
            int err = pthread_cancel(pids[i]);
            if (err < 0) {
                return err;
            }
            ++cnt;
        }
    }
    return cnt;
}

int raft_create_woofs() {
    int num_woofs = sizeof(RAFT_WOOF_TO_CREATE) / RAFT_NAME_LENGTH;
    int i;
    for (i = 0; i < num_woofs; i++) {
        if (WooFCreate(RAFT_WOOF_TO_CREATE[i], RAFT_WOOF_ELEMENT_SIZE[i], RAFT_WOOF_HISTORY_SIZE[i]) < 0) {
            fprintf(stderr, "failed to create woof %s\n", RAFT_WOOF_ELEMENT_SIZE[i]);
            return -1;
        }
    }
    return 0;
}

int raft_start_server(int members,
                      char woof_name[RAFT_NAME_LENGTH],
                      char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                      int observer,
                      int timeout_min,
                      int timeout_max,
                      int replicate_delay) {
    RAFT_SERVER_STATE server_state = {0};
    server_state.members = members;
    memcpy(server_state.member_woofs, member_woofs, sizeof(server_state.member_woofs));
    server_state.current_term = 0;
    server_state.role = RAFT_FOLLOWER;
    if (observer) {
        server_state.role = RAFT_OBSERVER;
    }
    memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
    server_state.commit_index = 0;
    server_state.current_config = RAFT_CONFIG_STATUS_STABLE;
    memcpy(server_state.woof_name, woof_name, sizeof(server_state.woof_name));
    memcpy(server_state.current_leader, server_state.woof_name, RAFT_NAME_LENGTH);
    server_state.observers = 0;
    server_state.timeout_min = timeout_min;
    server_state.timeout_max = timeout_max;
    server_state.replicate_delay = replicate_delay;

    unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
    printf("start_server server_state commit_index: %lu, %lu\n", server_state.commit_index, seq);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "Couldn't initialize server state\n");
        return -1;
    }

    if (monitor_create(RAFT_MONITOR_NAME) < 0) {
        fprintf(stderr, "Failed to create and start the handler monitor\n");
        return -1;
    }

    printf("Server started.\n");
    printf("WooF namespace: %s\n", server_state.woof_name);
    printf("Cluster has %d members:\n", server_state.members);
    int i;
    for (i = 0; i < server_state.members; ++i) {
        printf("%d: %s\n", i + 1, server_state.member_woofs[i]);
    }

    RAFT_HEARTBEAT heartbeat = {0};
    heartbeat.term = 0;
    heartbeat.timestamp = get_milliseconds();
    // experiment cheat
    // dht
    // if (strstr(woof_name, "169.231.23") != NULL) {
    //     heartbeat.timestamp = get_milliseconds() - timeout_min;
    // }
    // sed
    // if (strstr(woof_name, "128.111.39") != NULL) {
    //     heartbeat.timestamp = get_milliseconds() - timeout_min;
    // }
    // sed1
    // if (strstr(woof_name, "128.111.39.229") != NULL) {
    //     heartbeat.timestamp = 0;
    // }
    // raft_throught test: dht1, val1, sed1, dht4, dht5
    if (strstr(woof_name, "169.231.234.163") != NULL || strstr(woof_name, "128.111.45.112") != NULL ||
        strstr(woof_name, "128.111.39.229") != NULL || strstr(woof_name, "169.231.234.204") != NULL ||
        strstr(woof_name, "169.231.234.220") != NULL) {
        heartbeat.timestamp = get_milliseconds() - timeout_min;
    }
    seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "Couldn't put the first heartbeat\n");
        return -1;
    }
    printf("Put a heartbeat[%lu] %" PRIu64 "\n", seq, heartbeat.timestamp);

    RAFT_CLIENT_PUT_ARG client_put_arg = {0};
    client_put_arg.last_seqno = 0;
    seq = WooFPut(RAFT_CLIENT_PUT_ARG_WOOF, "h_client_put", &client_put_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "Couldn't start h_client_put\n");
        return -1;
    }
    RAFT_APPEND_ENTRIES_ROUTINE_ARG append_routine_arg = {0};
    append_routine_arg.last_seqno = 0;
    seq = monitor_put(RAFT_MONITOR_NAME, RAFT_APPEND_ENTRIES_ROUTINE_WOOF, "h_append_entries", &append_routine_arg, 1);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "Couldn't start h_append_entries\n");
        return -1;
    }
    if (!observer) {
        RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
        timeout_checker_arg.timeout_value = random_timeout(get_milliseconds(), timeout_min, timeout_max);
        seq = monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg, 1);
        if (WooFInvalid(seq)) {
            fprintf(stderr, "Couldn't start h_timeout_checker\n");
            return -1;
        }
    }
    printf("Started daemon functions\n");
    return 0;
}