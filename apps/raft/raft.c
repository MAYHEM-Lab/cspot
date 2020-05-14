#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include "raft.h"

FILE *raft_log_output;
int raft_log_level;

int get_server_state(RAFT_SERVER_STATE *server_state) {
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		return -1;
	}
	if (WooFGet(RAFT_SERVER_STATE_WOOF, server_state, last_server_state) < 0) {
		return -1;
	}
	return 0;
}

int random_timeout(unsigned long seed) {
	srand(seed);
	return RAFT_TIMEOUT_MIN + (rand() % (RAFT_TIMEOUT_MAX - RAFT_TIMEOUT_MIN));
}

unsigned long get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec / 1000;
}

int read_config(FILE *fp, int *members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
	char buffer[256];
	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		fprintf(stderr, "wrong format of config file\n");
		return -1;
	}
	*members = (int)strtol(buffer, (char **)NULL, 10);
	if (*members == 0) {
		fprintf(stderr, "wrong format of config file\n");
		return -1;
	}
	int i;
	for (i = 0; i < *members; ++i) {
		if (fgets(buffer, sizeof(buffer), fp) == NULL) {
			fprintf(stderr, "wrong format of config file\n");
			return -1;
		}
		buffer[strcspn(buffer, "\n")] = 0;
		if (buffer[strlen(buffer) - 1] == '/') {
			buffer[strlen(buffer) - 1] = 0;
		}
		strcpy(member_woofs[i], buffer);
	}
	return 0;
}

int node_woof_name(char *node_woof) {
	int err;
	char local_ip[25];
	char namespace[256];
	char *str;

	str = getenv("WOOFC_NAMESPACE");
	if (str == NULL) {
		getcwd(namespace, sizeof(namespace));
	} else {
		strncpy(namespace, str, sizeof(namespace));
	}
	err = WooFLocalIP(local_ip, sizeof(local_ip));
	if (err < 0) {
		fprintf(stderr, "no local IP\n");
		fflush(stderr);
		return -1;
	}
	sprintf(node_woof, "woof://%s%s", local_ip, namespace);
	return 0;
}

int member_id(char *woof_name, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
	int i;
	for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
		if (strcmp(woof_name, member_woofs[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int encode_config(char *dst, int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
	sprintf(dst, "%d;", members);
	int i;
	for (i = 0; i < members; ++i) {
		sprintf(dst + strlen(dst), "%s;", member_woofs[i]);
	}
	return strlen(dst);
}

int decode_config(char *src, int *members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
	int len = 0;
    char *token;
    token = strtok(src, ";");
	len += strlen(token) + 1;
    *members = (int)strtol(token, (char **)NULL, 10);
	if (*members == 0) {
		return -1;
	}
	int i;
	for (i = 0; i < *members; ++i) {
        token = strtok(NULL, ";");
        strcpy(member_woofs[i], token);
		len += strlen(token) + 1;
	}
	return len;
}

int qsort_strcmp(const void *a, const void *b) {
    return strcmp((const char*)a, (const char*)b);
}

int compute_joint_config(int old_members, char old_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
	int new_members, char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
	int *joint_members, char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
	
	if (old_members + new_members > RAFT_MAX_MEMBERS) {
		return -1;
	}
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

int threads_join(int members, pthread_t *pids) {
	int cnt = 0;
	int i;
	for (i = 0; i < members; ++i) {
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

int threads_cancel(int members, pthread_t *pids) {
	int cnt = 0;
	int i;
	for (i = 0; i < members; ++i) {
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

void log_set_tag(const char *tag) {
	strcpy(log_tag, tag);
}

void log_set_level(int level) {
	raft_log_level = level;
}

void log_set_output(FILE *file) {
	raft_log_output = file;
}

void log_debug(const char *message, ...) {
	if (raft_log_level > LOG_DEBUG) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(raft_log_output, "\033[0;34m");
	fprintf(raft_log_output, "DEBUG| %.19s:%.3d [%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(raft_log_output, message, argptr);
	fprintf(raft_log_output, "\033[0m\n");
    va_end(argptr);
}

void log_info(const char *message, ...) {
	if (raft_log_level > LOG_INFO) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(raft_log_output, "\033[0;32m");
	fprintf(raft_log_output, "INFO | %.19s:%.3d [%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(raft_log_output, message, argptr);
	fprintf(raft_log_output, "\033[0m\n");
    va_end(argptr);
}

void log_warn(const char *message, ...) {
	if (raft_log_level > LOG_WARN) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(raft_log_output, "\033[0;33m");
	fprintf(raft_log_output, "WARN | %.19s:%.3d [%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(raft_log_output, message, argptr);
	fprintf(raft_log_output, "\033[0m\n");
    va_end(argptr);
}

void log_error(const char *message, ...) {
	if (raft_log_level > LOG_ERROR) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(raft_log_output, "\033[0;31m");
	fprintf(raft_log_output, "ERROR| %.19s:%.3d [%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(raft_log_output, message, argptr);
	fprintf(raft_log_output, "\033[0m\n");
    va_end(argptr);
}

int create_woofs() {
	int num_woofs = sizeof(RAFT_WOOF_TO_CREATE) / RAFT_NAME_LENGTH;
	int i;
	for (i = 0; i < num_woofs; i++) {
		if (WooFCreate(RAFT_WOOF_TO_CREATE[i], RAFT_WOOF_ELEMENT_SIZE[i], RAFT_WOOF_HISTORY_SIZE) < 0) {
			fprintf(stderr, "failed to create woof %s\n", RAFT_WOOF_ELEMENT_SIZE[i]);
			return -1;
		}
	}
	return 0;
}

int start_server(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH], int observer) {
	RAFT_SERVER_STATE server_state;
	server_state.members = members;
	memcpy(server_state.member_woofs, member_woofs, sizeof(server_state.member_woofs));
	server_state.current_term = 0;
	server_state.role = RAFT_FOLLOWER;
	if (observer) {
		server_state.role = RAFT_OBSERVER;
	}
	memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
	server_state.commit_index = 0;
	server_state.current_config = RAFT_CONFIG_STABLE;
	if (node_woof_name(server_state.woof_name) < 0) {
		fprintf(stderr, "Couldn't get woof name\n");
	}
	memcpy(server_state.current_leader, server_state.woof_name, RAFT_NAME_LENGTH);
	server_state.observers = 0;

	unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
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

	RAFT_HEARTBEAT heartbeat;
	heartbeat.term = 0;
	heartbeat.timestamp = get_milliseconds();
	seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't put the first heartbeat\n");
		return -1;
	}
	printf("Put a heartbeat\n");

	RAFT_CLIENT_PUT_ARG client_put_arg;
	client_put_arg.last_seqno = 0;
	seq = monitor_put(RAFT_MONITOR_NAME, RAFT_CLIENT_PUT_ARG_WOOF, "h_client_put", &client_put_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't start h_client_put\n");
		return -1;
	}
	if (!observer) {
		RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg;
		timeout_checker_arg.timeout_value = random_timeout(get_milliseconds());
		seq = monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "Couldn't start h_timeout_checker\n");
			return -1;
		}
	}
	printf("Started daemon functions\n");
	return 0;
}