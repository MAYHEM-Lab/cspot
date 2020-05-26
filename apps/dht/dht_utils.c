#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include "dht.h"
#include "dht_utils.h"

char log_tag[DHT_NAME_LENGTH];
FILE *log_output;
int log_level;
extern char WooF_dir[2048];

unsigned long get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec / 1000;
}

int node_woof_name(char *woof_name) {
	char *str = getenv("WOOFC_NAMESPACE");
	char namespace[DHT_NAME_LENGTH];
	if (str == NULL) {
		getcwd(namespace, sizeof(namespace));
	} else {
		strncpy(namespace, str, sizeof(namespace));
	}
	char local_ip[25];
	if (WooFLocalIP(local_ip, sizeof(local_ip)) < 0) {
		sprintf(dht_error_msg, "no local IP");
		return -1;
	}
	sprintf(woof_name, "woof://%s%s", local_ip, namespace);
	return 0;
}

void log_set_tag(const char *tag) {
	strcpy(log_tag, tag);
}

void log_set_level(int level) {
	log_level = level;
}

void log_set_output(FILE *file) {
	log_output = file;
}

void log_debug(const char *message, ...) {
	if (log_level > DHT_LOG_DEBUG) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;34m");
	fprintf(log_output, "DEBUG| %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_info(const char *message, ...) {
	if (log_level > DHT_LOG_INFO) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;32m");
	fprintf(log_output, "INFO | %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_warn(const char *message, ...) {
	if (log_level > DHT_LOG_WARN) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;33m");
	fprintf(log_output, "WARN | %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_error(const char *message, ...) {
	if (log_level > DHT_LOG_ERROR) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;31m");
	fprintf(log_output, "ERROR| %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

int get_latest_element(char *woof_name, void *element) {
	unsigned long latest_seq = WooFGetLatestSeqno(woof_name);
	if (WooFInvalid(latest_seq)) {
		sprintf(dht_error_msg, "failed to get the latest seqno from %s", woof_name);
		return -1;
	}
	if (WooFGet(woof_name, element, latest_seq) < 0) {
		sprintf(dht_error_msg, "failed to get the latest woof element");
		return -1;
	}
	return 0;
}

int read_config(FILE *fp, int *len, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
	char buffer[256];
	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		sprintf(dht_error_msg, "wrong format of config file\n");
		return -1;
	}
	*len = (int)strtol(buffer, (char **)NULL, 10);
	if (*len == 0) {
		sprintf(dht_error_msg, "wrong format of config file\n");
		return -1;
	}
	if (*len > DHT_REPLICA_NUMBER) {
		sprintf(dht_error_msg, "maximum number of replica is %d, have %d", DHT_REPLICA_NUMBER, *len);
		return -1;
	}
	memset(replicas, 0, DHT_REPLICA_NUMBER * DHT_NAME_LENGTH);
	int i;
	for (i = 0; i < *len; ++i) {
		if (fgets(buffer, sizeof(buffer), fp) == NULL) {
			sprintf(dht_error_msg, "wrong format of config file\n");
			return -1;
		}
		buffer[strcspn(buffer, "\n")] = 0;
		if (buffer[strlen(buffer) - 1] == '/') {
			buffer[strlen(buffer) - 1] = 0;
		}
		strcpy(replicas[i], buffer);
	}
	return 0;
}

void dht_init(unsigned char *node_hash, char *node_addr, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH], int replica_id, DHT_TABLE *dht_table) {
	memcpy(dht_table->node_hash, node_hash, sizeof(dht_table->node_hash));
	memset(dht_table->finger_hash, 0, sizeof(dht_table->finger_hash));
	memset(dht_table->successor_hash, 0, sizeof(dht_table->successor_hash));
	memcpy(dht_table->successor_hash[0], node_hash, sizeof(dht_table->successor_hash[0]));
	memset(dht_table->predecessor_hash, 0, sizeof(dht_table->predecessor_hash));
	memcpy(dht_table->node_addr, node_addr, sizeof(dht_table->node_addr));
	memcpy(dht_table->replicas, replicas, sizeof(dht_table->replicas));
	dht_table->replica_id = replica_id;
}

void dht_init_find_arg(DHT_FIND_SUCCESSOR_ARG *arg, char *key, char *hashed_key, char *callback_namespace) {
	arg->hops = 0;
	memcpy(arg->key, key, sizeof(arg->key));
	memcpy(arg->hashed_key, hashed_key, sizeof(arg->hashed_key));
	strcpy(arg->callback_namespace, callback_namespace);
}

void print_node_hash(char *dst, const unsigned char *id_hash) {
	int i;
	sprintf(dst, "");
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(dst, "%s%02X", dst, id_hash[i]);
	}
}

int is_empty(char hash[SHA_DIGEST_LENGTH]) {
	int i;
	for (i = 0; i < SHA_DIGEST_LENGTH; ++i) {
		if (hash[i] != 0) {
			return 0;
		}
	}
	return 1;
}

// if n in (lower, upper)
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

void shift_successor_list(unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH]) {
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i){
		memcpy(successor_hash[i], successor_hash[i + 1], sizeof(successor_hash[i]));
	}
	memset(successor_hash[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor_hash[DHT_SUCCESSOR_LIST_R - 1]));
}

int hmap_set(char *hash, DHT_HASHMAP_ENTRY *entry) {
	if (hash == NULL || entry == NULL) {
		sprintf(dht_error_msg, "hash or address is null");
		return -1;
	}

	char woof_name[strlen(DHT_HASHMAP_WOOF) + DHT_NAME_LENGTH];
	strcpy(woof_name, DHT_HASHMAP_WOOF);
	print_node_hash(woof_name + strlen(DHT_HASHMAP_WOOF), hash);
	if (!WooFExist(woof_name)) {
		if (WooFCreate(woof_name, sizeof(DHT_HASHMAP_ENTRY), DHT_HISTORY_LENGTH_SHORT) < 0) {
			sprintf(dht_error_msg, "failed to create hashmap entry woof %s", woof_name);
			return -1;
		}
		char woof_file[DHT_NAME_LENGTH];
		sprintf(woof_file, "%s/%s", WooF_dir, woof_name);
		if (chmod(woof_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
			sprintf(dht_error_msg, "failed to change file %s's mode to 0666", woof_name);
			return -1;
		}
	}

	unsigned long seq = WooFPut(woof_name, NULL, entry);
	if (WooFInvalid(seq)) {
		sprintf(dht_error_msg, "failed to write hashmap entry to woof");
		return -1;
	}
	return 0;
}

int hmap_get(char *hash, DHT_HASHMAP_ENTRY *entry) {
	if (hash == NULL || entry == NULL) {
		sprintf(dht_error_msg, "hash or address is null");
		return -1;
	}
	if (is_empty(hash)) {
		sprintf(dht_error_msg, "hash is zero");
		return -1;
	}

	char woof_name[strlen(DHT_HASHMAP_WOOF) + DHT_NAME_LENGTH];
	strcpy(woof_name, DHT_HASHMAP_WOOF);
	print_node_hash(woof_name + strlen(DHT_HASHMAP_WOOF), hash);
	if (!WooFExist(woof_name)) {
		sprintf(dht_error_msg, "hashmap entry %s doesn't exist", woof_name + strlen(DHT_HASHMAP_WOOF));
		return -1;
	}

	if (get_latest_element(woof_name, entry) < 0) {
		sprintf(dht_error_msg, "failed to get the latest hashmap entry: %s", dht_error_msg);
		return -1;
	}
	
	return 0;
}

int hmap_set_replicas(char *hash, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH], int leader) {
	DHT_HASHMAP_ENTRY entry = {0};
	entry.leader = 0;
	memcpy(entry.replicas, replicas, sizeof(entry.replicas));
	return hmap_set(hash, &entry);
}