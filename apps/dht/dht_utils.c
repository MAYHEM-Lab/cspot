#include "dht_utils.h"

#include "dht.h"
#ifdef USE_RAFT
#include "raft.h"
#include "raft_client.h"
#endif

#include <openssl/sha.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

static const char zero_hash[SHA_DIGEST_LENGTH] = {0};

char log_tag[DHT_NAME_LENGTH];
FILE* log_output;
int log_level;
extern char WooF_dir[2048];

uint64_t get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

void node_woof_namespace(char* woof_namespace) {
    char* str = getenv("WOOFC_NAMESPACE");
    char ns[DHT_NAME_LENGTH];
    if (str == NULL) {
        getcwd(ns, sizeof(ns));
    } else {
        strncpy(ns, str, sizeof(ns));
    }
    strcpy(woof_namespace, ns);
}

void log_set_tag(const char* tag) {
    strcpy(log_tag, tag);
}

void log_set_level(int level) {
    log_level = level;
}

void log_set_output(FILE* file) {
    log_output = file;
}

void log_debug(const char* message, ...) {
    if (log_output == NULL || log_level > DHT_LOG_DEBUG) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;34m");
    fprintf(log_output, "DEBUG| %.19s:%.3d [dht:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_info(const char* message, ...) {
    if (log_output == NULL || log_level > DHT_LOG_INFO) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;32m");
    fprintf(log_output, "INFO | %.19s:%.3d [dht:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_warn(const char* message, ...) {
    if (log_output == NULL || log_level > DHT_LOG_WARN) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;33m");
    fprintf(log_output, "WARN | %.19s:%.3d [dht:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_error(const char* message, ...) {
    if (log_output == NULL || log_level > DHT_LOG_ERROR) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;31m");
    fprintf(log_output, "ERROR| %.19s:%.3d [dht:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

int get_latest_element(char* woof_name, void* element) {
    unsigned long latest_seq = WooFGetLatestSeqno(woof_name);
    if (WooFInvalid(latest_seq)) {
        sprintf(dht_error_msg, "failed to get the latest seqno from %s", woof_name);
        return -1;
    }
    if (WooFGet(woof_name, element, latest_seq) < 0) {
        sprintf(dht_error_msg, "failed to get the latest woof element from %s[%lu]", woof_name, latest_seq);
        return -1;
    }
    return 0;
}

int read_raft_config(FILE* fp, char* name, int* len, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    strcpy(name, buffer);
    if (name[strlen(name) - 1] == '\n') {
        name[strlen(name)] = 0;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    *len = (int)strtol(buffer, (char**)NULL, 10);
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

int read_dht_config(FILE* fp,
                    int* stabilize_freq,
                    int* chk_predecessor_freq,
                    int* fix_finger_freq,
                    int* update_leader_freq,
                    int* daemon_wakeup_freq) {
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    *stabilize_freq = (int)strtol(buffer, (char**)NULL, 10);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    *chk_predecessor_freq = (int)strtol(buffer, (char**)NULL, 10);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    *fix_finger_freq = (int)strtol(buffer, (char**)NULL, 10);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    *update_leader_freq = (int)strtol(buffer, (char**)NULL, 10);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(dht_error_msg, "wrong format of config file\n");
        return -1;
    }
    *daemon_wakeup_freq = (int)strtol(buffer, (char**)NULL, 10);
    return 0;
}

void serialize_dht_config(char* dst,
                          int stabilize_freq,
                          int chk_predecessor_freq,
                          int fix_finger_freq,
                          int update_leader_freq,
                          int daemon_wakeup_freq) {
    sprintf(dst,
            "%d %d %d %d %d",
            stabilize_freq,
            chk_predecessor_freq,
            fix_finger_freq,
            update_leader_freq,
            daemon_wakeup_freq);
}
void deserialize_dht_config(char* src,
                            int* stabilize_freq,
                            int* chk_predecessor_freq,
                            int* fix_finger_freq,
                            int* update_leader_freq,
                            int* daemon_wakeup_freq) {
    sscanf(src,
           "%d %d %d %d %d",
           stabilize_freq,
           chk_predecessor_freq,
           fix_finger_freq,
           update_leader_freq,
           daemon_wakeup_freq);
}

int dht_init(unsigned char* node_hash,
             char* node_name,
             char* node_addr,
             char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (strcmp(node_addr, node_replicas[i]) == 0) {
            break;
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        sprintf(dht_error_msg, "node_addr is not part of node_replicas");
        return -1;
    }
    DHT_NODE_INFO node_info = {0};
    memcpy(node_info.name, node_name, sizeof(node_info.name));
    memcpy(node_info.hash, node_hash, sizeof(node_info.hash));
    memcpy(node_info.addr, node_addr, sizeof(node_info.addr));
    memcpy(node_info.replicas, node_replicas, sizeof(node_info.replicas));
    node_info.replica_id = i;
    node_info.leader_id = i;
    unsigned long seq = WooFPut(DHT_NODE_INFO_WOOF, NULL, &node_info);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to initialize %s", DHT_NODE_INFO_WOOF);
        return -1;
    }

    DHT_PREDECESSOR_INFO predecessor_info = {0};
    seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor_info);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to initialize %s", DHT_PREDECESSOR_INFO_WOOF);
        return -1;
    }

    DHT_SUCCESSOR_INFO successor_info = {0};
    // initialize successor to itself
    memcpy(successor_info.hash, node_hash, sizeof(node_info.hash));
    memcpy(successor_info.replicas[0], node_replicas, sizeof(successor_info.replicas[0]));
    seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor_info);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to initialize %s", DHT_SUCCESSOR_INFO_WOOF);
        return -1;
    }

    DHT_FINGER_INFO finger_info = {0};
    for (i = 1; i < SHA_DIGEST_LENGTH * 8 + 1; ++i) {
        seq = set_finger_info(i, &finger_info);
        if (WooFInvalid(seq)) {
            sprintf(dht_error_msg, "failed to initialize %s%d", DHT_FINGER_INFO_WOOF, i);
            return -1;
        }
    }

    return 0;
}

void dht_init_find_arg(DHT_FIND_SUCCESSOR_ARG* arg, char* key, char* hashed_key, char* callback_namespace) {
    memset(arg, 0, sizeof(DHT_FIND_SUCCESSOR_ARG));
    memcpy(arg->key, key, sizeof(arg->key));
    memcpy(arg->hashed_key, hashed_key, sizeof(arg->hashed_key));
    strcpy(arg->callback_namespace, callback_namespace);
}

char* dht_hash(unsigned char* dst, char* src) {
    return SHA1(src, strlen(src), dst);
}

void print_node_hash(char* dst, const unsigned char* id_hash) {
    int i;
    sprintf(dst, "");
    for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(dst, "%s%02X", dst, id_hash[i]);
    }
}

int is_empty(char hash[SHA_DIGEST_LENGTH]) {
    return memcmp(hash, zero_hash, SHA_DIGEST_LENGTH) == 0;
}

// if n in (lower, upper)
int in_range(unsigned char* n, unsigned char* lower, unsigned char* upper) {
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

int get_latest_node_info(DHT_NODE_INFO* element) {
    return get_latest_element(DHT_NODE_INFO_WOOF, element);
}

int get_latest_predecessor_info(DHT_PREDECESSOR_INFO* element) {
    return get_latest_element(DHT_PREDECESSOR_INFO_WOOF, element);
}

int get_latest_successor_info(DHT_SUCCESSOR_INFO* element) {
    return get_latest_element(DHT_SUCCESSOR_INFO_WOOF, element);
}

int get_latest_finger_info(int finger_id, DHT_FINGER_INFO* element) {
    char woof_name[DHT_NAME_LENGTH];
    sprintf(woof_name, "%s%d", DHT_FINGER_INFO_WOOF, finger_id);
    return get_latest_element(woof_name, element);
}

unsigned long set_finger_info(int finger_id, DHT_FINGER_INFO* element) {
    char woof_name[DHT_NAME_LENGTH];
    sprintf(woof_name, "%s%d", DHT_FINGER_INFO_WOOF, finger_id);
    return WooFPut(woof_name, NULL, element);
}

char* predecessor_addr(DHT_PREDECESSOR_INFO* info) {
    return info->replicas[info->leader];
}

char* successor_addr(DHT_SUCCESSOR_INFO* info, int r) {
    return info->replicas[r][info->leader[r]];
}

char* finger_addr(DHT_FINGER_INFO* info) {
    return info->replicas[info->leader];
}

#ifdef USE_RAFT
int raft_leader_id() {
    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        sprintf(dht_error_msg, "couldn't get latest node info: %s", dht_error_msg);
        return -1;
    }

    RAFT_SERVER_STATE raft_state = {0};
    if (get_server_state(&raft_state) < 0) {
        sprintf(dht_error_msg, "failed to get RAFT's server_state: %s", raft_error_msg);
        return -1;
    }
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (strcmp(raft_state.current_leader, node.replicas[i]) == 0) {
            break;
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        sprintf(dht_error_msg, "current_leader %s is not in the replica list", raft_state.current_leader);
        return -1;
    }
    if (i == node.replica_id && raft_state.role != RAFT_LEADER) {
        sprintf(dht_error_msg, "the node's cluster has no leader");
        return -1;
    }
    return i;
}

int invalidate_fingers(char hash[SHA_DIGEST_LENGTH]) {
    int i;
    for (i = 1; i <= SHA_DIGEST_LENGTH * 8; ++i) {
        DHT_FINGER_INFO finger = {0};
        if (get_latest_finger_info(i, &finger) < 0) {
            log_error("failed to get finger[%d]'s info: %s", i, dht_error_msg);
            continue;
        }
        if (memcmp(finger.hash, hash, sizeof(finger.hash)) == 0) {
            memset(&finger, 0, sizeof(finger));
            unsigned long seq = set_finger_info(i, &finger);
            if (WooFInvalid(seq)) {
                log_error("failed to invalidate finger[%d]", i);
                continue;
            }
        }
    }
    return 0;
}
#endif