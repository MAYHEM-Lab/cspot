#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

FILE *raft_log_output;
int dht_log_level;

void dht_init(unsigned char *node_hash, char *node_addr, DHT_TABLE_EL *el) {
	memcpy(el->node_hash, node_hash, sizeof(el->node_hash));
	memcpy(el->node_addr, node_addr, sizeof(el->node_addr));
	memset(el->finger_addr, 0, sizeof(el->finger_addr));
	memset(el->finger_hash, 0, sizeof(el->finger_hash));
	memset(el->successor_addr, 0, sizeof(el->successor_addr));
	memset(el->successor_hash, 0, sizeof(el->successor_hash));
	memcpy(el->successor_hash[0], node_hash, sizeof(el->successor_hash[0]));
	memcpy(el->successor_addr[0], node_addr, sizeof(el->successor_addr[0]));
	memset(el->predecessor_addr, 0, sizeof(el->predecessor_addr));
	memset(el->predecessor_hash, 0, sizeof(el->predecessor_hash));
}

void find_init(char *node_addr, char *callback, char *topic, FIND_SUCESSOR_ARG *arg) {
	arg->request_seq_no = 0;
	arg->finger_index = 0;
	arg->hops = 0;
	SHA1(topic, strlen(topic), arg->id_hash);
	sprintf(arg->callback_woof, "%s/%s", node_addr, DHT_FIND_SUCCESSOR_RESULT_WOOF);
	strcpy(arg->callback_handler, callback);
}

unsigned long get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec / 1000;
}

int node_woof_name(char *node_woof) {
	char *str = getenv("WOOFC_NAMESPACE");
	char namespace[DHT_NAME_LENGTH];
	if (str == NULL) {
		getcwd(namespace, sizeof(namespace));
	} else {
		strncpy(namespace, str, sizeof(namespace));
	}
	char local_ip[25];
	if (WooFLocalIP(local_ip, sizeof(local_ip)) < 0) {
		fprintf(stderr, "no local IP\n");
		return -1;
	}
	sprintf(node_woof, "woof://%s%s", local_ip, namespace);
	return 0;
}

void print_node_hash(char *dst, const unsigned char *id_hash) {
	int i;
	sprintf(dst, "");
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(dst, "%s%02X", dst, id_hash[i]);
	}
}

int in_range(unsigned char *n, unsigned char *lower, unsigned char *upper) {
	if (memcmp(lower, upper, SHA_DIGEST_LENGTH) < 0) {
		if (memcmp(lower, n, SHA_DIGEST_LENGTH) < 0 && memcmp(n, upper, SHA_DIGEST_LENGTH) < 0) {
			return 1;
		}
		return 0;
	}
	if (memcmp(lower, n, SHA_DIGEST_LENGTH) < 0 || memcmp(n, upper, SHA_DIGEST_LENGTH) < 0) {
		return 1;
	}
	return 0;
}

void shift_successor_list(char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH], unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH]) {
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i){
		memcpy(successor_addr[i], successor_addr[i + 1], sizeof(successor_addr[i]));
		memcpy(successor_hash[i], successor_hash[i + 1], sizeof(successor_hash[i]));
	}
	memset(successor_addr[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor_addr[DHT_SUCCESSOR_LIST_R - 1]));
	memset(successor_hash[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor_hash[DHT_SUCCESSOR_LIST_R - 1]));
}

void log_set_tag(const char *tag) {
	strcpy(log_tag, tag);
}

void log_set_level(int level) {
	dht_log_level = level;
}

void log_set_output(FILE *file) {
	raft_log_output = file;
}

void log_debug(const char *message, ...) {
	if (dht_log_level > LOG_DEBUG) {
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
	if (dht_log_level > LOG_INFO) {
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
	if (dht_log_level > LOG_WARN) {
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
	if (dht_log_level > LOG_ERROR) {
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