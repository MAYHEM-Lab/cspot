#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "raft.h"

FILE *raft_log_output;
int raft_log_level;

int random_timeout(unsigned long seed) {
	srand(seed);
	return RAFT_TIMEOUT_MIN + (rand() % (RAFT_TIMEOUT_MAX - RAFT_TIMEOUT_MIN));
}

unsigned long get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec / 1000;
}

void read_config(FILE *fp, int *members, char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH]) {
	char buffer[256];
	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		fprintf(stderr, "Wrong format of config file\n");
		fflush(stderr);
		exit(1);
	}
	*members = atoi(buffer);
	int i;
	for (i = 0; i < *members; ++i) {
		if (fgets(buffer, sizeof(buffer), fp) == NULL) {
			fprintf(stderr, "Wrong format of config file\n");
			fflush(stderr);
			exit(1);
		}
		buffer[strcspn(buffer, "\n")] = 0;
		if (buffer[strlen(buffer) - 1] == '/') {
			buffer[strlen(buffer) - 1] = 0;
		}
		strncpy(member_woofs[i], buffer, RAFT_WOOF_NAME_LENGTH);
	}
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

int encode_config(int members, char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH], RAFT_DATA_TYPE *data) {
	sprintf(data->val, "%d;", members);
	int i;
	for (i = 0; i < members; ++i) {
		if (strlen(data->val) + strlen(member_woofs[i]) + 1 > RAFT_DATA_TYPE_SIZE) {
			return -1; // RAFT_DATA_TYPE can't accommodate encoded config
		}
		sprintf(data->val + strlen(data->val), "%s;", member_woofs[i]);
	}
	return 0;
}

int decode_config(RAFT_DATA_TYPE data, int *members, char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH]) {
    char *token;
    token = strtok(data.val, ";");
    *members = atoi(token);
	int i;
	for (i = 0; i < *members; ++i) {
        token = strtok(NULL, ";");
        strcpy(member_woofs[i], token);
	}
	return 0;
}

int qsort_strcmp(const void *a, const void *b) {
    return strcmp((const char*)a, (const char*)b);
}

int compute_joint_config(int old_members, char old_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH],
	int new_members, char new_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH],
	int *joint_members, char joint_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH]) {
	
	if (old_members + new_members > RAFT_MAX_SERVER_NUMBER) {
		return -1;
	}
	int i;
	for (i = 0; i < old_members; ++i) {
		strcpy(joint_member_woofs[i], old_member_woofs[i]);
	}
    for (i = 0; i < new_members; ++i) {
		strcpy(joint_member_woofs[old_members + i], new_member_woofs[i]);
	}
	qsort(joint_member_woofs, old_members + new_members, RAFT_WOOF_NAME_LENGTH, qsort_strcmp);
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