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
    raft_client_timeout = 0;
    raft_client_result_delay = 10;
}

void raft_set_client_timeout(int timeout) {
    raft_client_timeout = timeout;
}

int raft_put(RAFT_DATA_TYPE *data, unsigned long *index, unsigned long *term, bool sync) {
    log_set_tag("raft_put");
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_ARG arg;
    memcpy(&arg.data, data, sizeof(RAFT_DATA_TYPE)); 
    char woof_name[RAFT_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_ARG_WOOF);
	unsigned long seq = WooFPut(woof_name, NULL, &arg);
	if (WooFInvalid(seq)) {
		log_error("failed to send put request");
		return -1;
	}
log_debug("%lums to WooFPut", get_milliseconds() - begin);

	RAFT_CLIENT_PUT_RESULT result;
    bool appended = false;
    while (appended == false) {
        if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
            log_error("timeout after %lums", get_milliseconds() - begin);
            return -1;
        }
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
        unsigned long latest_result = 0;
        while (latest_result < seq) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                log_error("timeout after %lums", get_milliseconds() - begin);
                return -1;
            }
            latest_result = WooFGetLatestSeqno(woof_name);
            usleep(raft_client_result_delay * 1000);
        }
log_debug("%lums to WooFGetLatestSeqno", get_milliseconds() - begin);
        int err = WooFGet(woof_name, &result, seq);
        if (err < 0) {
            log_error("failed to get put result for put request %lu", seq);
            return -1;
        }
log_debug("%lums to WooFGet", get_milliseconds() - begin);
        if (result.appended == false) {
            log_warn("not a leader, redirect to %s", result.current_leader);
            memcpy(raft_client_leader, result.current_leader, RAFT_WOOF_NAME_LENGTH);
        }
        appended = result.appended;
    }
	
    if (sync) {
        RAFT_SERVER_STATE server_state;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_SERVER_STATE_WOOF);
        unsigned long commit_index = 0;
        while (commit_index < result.seq_no) {
            if (raft_client_timeout > 0 && get_milliseconds() - begin > raft_client_timeout) {
                log_error("timeout after %lums", get_milliseconds() - begin);
                return -1;
            }
            unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
            int err = WooFGet(woof_name, &server_state, latest_server_state);
            if (err < 0) {
                log_error("appended but can't get leader's commit index");
                return -1;
            }
            commit_index = server_state.commit_index;
            usleep(raft_client_result_delay * 1000);
        }

        RAFT_LOG_ENTRY entry;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
        int err = WooFGet(woof_name, &entry, result.seq_no);
        if (err < 0) {
            log_error("appended but can't check committed entry term");
            return -1;
        }
        if (entry.term == result.term) {
            log_debug("entry %lu committed at term %lu, val: %s, took %lums", result.seq_no, entry.term, entry.data.val, get_milliseconds() - begin);
            if (index != NULL) {
                *index = result.seq_no;
            }
            if (term != NULL) {
                *term = entry.term;
            }
            return 0;
        } else {
            log_warn("entry %lu appended but got overriden at term %lu, val: %s", result.seq_no, entry.term, entry.data.val);
            return -1;
        }
    } else {
        log_debug("entry %lu appended and is pending for committing at term %lu, took %lums", result.seq_no, result.term, get_milliseconds() - begin);
        if (index != NULL) {
            *index = result.seq_no;
        }
        if (term != NULL) {
            *term = result.term;
        }
        return 0;
    }
}

int raft_get(RAFT_DATA_TYPE *data, unsigned long index, unsigned term) {
    log_set_tag("raft_get");
    unsigned long begin = get_milliseconds();

    RAFT_CLIENT_PUT_RESULT result;
	char woof_name[RAFT_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_CLIENT_PUT_RESULT_WOOF);
	unsigned long latest_result = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_result)) {
        log_error("failed to get client put result at %lu", index);
        return -1;
    }
	if (latest_result < index) {
		log_debug("client put request still pending");
		return -1;
	}
	int err = WooFGet(woof_name, &result, index);
	if (err < 0) {
		log_error("can't get put result at %lu", index);
        return -1;
	}
	if (result.appended == false) {
		log_error("client put request got denied");
		return -1;
	} else {
        if (term > 0 && result.term != term) {
            log_error("this request is processed at term %lu not %lu", result.term, term);
            return -1;
        }
        RAFT_LOG_ENTRY entry;
        sprintf(woof_name, "%s/%s", raft_client_leader, RAFT_LOG_ENTRIES_WOOF);
        err = WooFGet(woof_name, &entry, result.seq_no);
        if (err < 0) {
            log_error("can't get log entry at %lu", result.seq_no);
            return -1;
        }
        if (entry.term != term) {
            log_error("entry term %lu doesn't match with request term %lu, it got overriden", entry.term, term);
            return -1;
        }
        memcpy(data, &entry.data, sizeof(RAFT_DATA_TYPE));
		log_debug("request %lu committed an entry at %lu at term %lu: %s", index, result.seq_no, result.term, data->val);
		return 0;
	}
}