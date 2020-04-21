#include <time.h>
#include <string.h>
#include "client.h"
#include "raft.h"

void raft_init_client(FILE *fp) {
    log_set_tag("raft_init_client");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

    read_config(fp, &raft_client_members, raft_client_servers);
    log_debug("%d servers", raft_client_members);
    int i;
    for (i = 0; i < raft_client_members; ++i) {
        log_debug("%d: %s", i, raft_client_servers[i]);
    }
    strcpy(raft_client_leader, raft_client_servers[0]);
    raft_client_timeout = 300;
    raft_client_result_delay = 10;
}

void raft_set_client_leader(char *leader) {
    strcpy(raft_client_leader, leader);
}

void raft_set_client_timeout(int timeout) {
    raft_client_timeout = timeout;
}

int raft_put(RAFT_DATA_TYPE *data, unsigned long *index, unsigned long *term, bool sync) {
    log_set_tag("raft_put");
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_ARG arg;
    arg.is_config = false;
    memcpy(&arg.data, data, sizeof(RAFT_DATA_TYPE)); 
    char handler_name[RAFT_WOOF_NAME_LENGTH];
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(handler_name, "%s/%s", raft_client_leader, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_ARG_WOOF);
    unsigned long r_seq = monitor_remote_put(handler_name, woof_name, "h_client_put", &arg);
    if (WooFInvalid(r_seq)) {
        log_error("failed to send put request");
        return RAFT_ERROR;
    }
    if (sync) {
        RAFT_CLIENT_PUT_RESULT result;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
        unsigned long latest_result = 0;
        while (latest_result < r_seq) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                log_error("timeout after %lums", raft_client_timeout);
                return RAFT_TIMEOUT;
            }
            latest_result = WooFGetLatestSeqno(woof_name);
            usleep(raft_client_result_delay * 1000);
        }
        log_debug("successfully put, waiting for commit");
        if (WooFGet(woof_name, &result, r_seq) < 0) {
            log_error("failed to get put result for put request %lu", r_seq);
            return RAFT_ERROR;
        }
        if (result.redirected == true) {
            log_warn("not a leader, redirect to %s", result.current_leader);
            memcpy(raft_client_leader, result.current_leader, RAFT_WOOF_NAME_LENGTH);
            return RAFT_REDIRECTED;
        }
        
        RAFT_SERVER_STATE server_state;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
        unsigned long commit_index = 0;
        while (commit_index < result.seq_no) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                log_error("timeout after %lums", raft_client_timeout);
                return RAFT_TIMEOUT;
            }
            unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
            if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
                log_error("appended but can't get leader's commit index");
                return RAFT_ERROR;
            }
            commit_index = server_state.commit_index;
            usleep(raft_client_result_delay * 1000);
        }

        RAFT_LOG_ENTRY entry;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
        if (WooFGet(woof_name, &entry, result.seq_no) < 0) {
            log_error("appended but can't check committed entry term");
            return RAFT_ERROR;
        }
        if (entry.term == result.term) {
            log_debug("entry %lu committed at term %lu, val: %s, took %lums", result.seq_no, entry.term, entry.data.val, get_milliseconds() - begin);
            if (index != NULL) {
                *index = result.seq_no;
            }
            if (term != NULL) {
                *term = entry.term;
            }
            return RAFT_SUCCESS;
        } else {
            log_warn("entry %lu appended but got overriden at term %lu, val: %s", result.seq_no, entry.term, entry.data.val);
            return RAFT_ERROR;
        }
    } else {
        log_debug("entry appended and is pending, took %lums", get_milliseconds() - begin);
        return RAFT_SUCCESS;
    }
}

int raft_get(RAFT_DATA_TYPE *data, unsigned long index, unsigned long term) {
    log_set_tag("raft_get");
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_RESULT result;
	char woof_name[RAFT_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
	unsigned long latest_result = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_result)) {
        log_error("failed to get client put result at %lu", index);
        return RAFT_ERROR;
    }
	if (latest_result < index) {
		log_debug("client put request still pending");
		return RAFT_ERROR;
	}
	if (WooFGet(woof_name, &result, index) < 0) {
		log_error("can't get put result at %lu", index);
        return RAFT_ERROR;
	}
	if (result.redirected == true) {
		log_error("client put request got redirected");
		return RAFT_REDIRECTED;
	} else {
        if (term > 0 && result.term != term) {
            log_error("this request is processed at term %lu not %lu", result.term, term);
            return RAFT_ERROR;
        }

        // check if it's committed
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
        unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
        if (WooFInvalid(latest_server_state)) {
            log_error("can't get the latest server state seqno");
            return RAFT_ERROR;
        }
        RAFT_SERVER_STATE server_state;
        if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
            log_error("can't get the server state");
            return RAFT_ERROR;
        }
        if (server_state.commit_index < result.seq_no) {
            log_debug("entry not committed yet");
            return RAFT_NOT_COMMITTED;
        }

        // check log entry term
        RAFT_LOG_ENTRY entry;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
        if (WooFGet(woof_name, &entry, result.seq_no) < 0) {
            log_error("can't get log entry at %lu", result.seq_no);
            return RAFT_ERROR;
        }
        if (term > 0 && entry.term != term) {
            log_error("entry term %lu doesn't match with request term %lu, it got overriden", entry.term, term);
            return RAFT_ERROR;
        }
        memcpy(data, &entry.data, sizeof(RAFT_DATA_TYPE));
		log_debug("request %lu committed an entry at %lu at term %lu: %s", index, result.seq_no, result.term, data->val);
		return RAFT_SUCCESS;
	}
}

int raft_reconfig(int members, char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH], unsigned long *index, unsigned long *term) {
    log_set_tag("raft_reconfig");
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_ARG arg;
    if (encode_config(arg.data.val, members, member_woofs) < 0) {
        log_error("couldn't encode new config");
        return RAFT_ERROR;
    }
    arg.is_config = true;
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_ARG_WOOF);
    unsigned long seq = WooFPut(woof_name, NULL, &arg);
    if (WooFInvalid(seq)) {
        log_error("failed to send put request");
        return RAFT_ERROR;
    }

    RAFT_CLIENT_PUT_RESULT result;
    sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
    unsigned long latest_result = 0;
    while (latest_result < seq) {
        if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
            log_error("timeout after %lums", get_milliseconds() - begin);
            return RAFT_TIMEOUT;
        }
        latest_result = WooFGetLatestSeqno(woof_name);
        usleep(raft_client_result_delay * 1000);
    }
    if (WooFGet(woof_name, &result, seq) < 0) {
        log_error("failed to get put result for put request %lu", seq);
        return RAFT_ERROR;
    }
    if (result.redirected == true) {
        log_warn("not a leader, redirect to %s", result.current_leader);
        memcpy(raft_client_leader, result.current_leader, RAFT_WOOF_NAME_LENGTH);
        return RAFT_REDIRECTED;
    }
    
    log_debug("reconfig requested, seq_no: %lu, term: %lu", result.seq_no, result.term);
    *index = result.seq_no;
    *term = result.term;
    return RAFT_SUCCESS;
}

int raft_current_leader(char *member_woof, char *current_leader) {
    char woof_name[RAFT_WOOF_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", member_woof, RAFT_SERVER_STATE_WOOF);
    unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_server_state)) {
        fprintf(stderr, "failed to get the latest seqno from %s\n", woof_name);
        fflush(stderr);
        return -1;
    }
    RAFT_SERVER_STATE server_state;
    if (WooFGet(woof_name, &server_state, latest_server_state) < 0) {
        fprintf(stderr, "failed to get the latest server_state %lu from %s\n", latest_server_state, woof_name);
        fflush(stderr);
        return -1;
    }
    strcpy(current_leader, server_state.current_leader);
    return 0;
}