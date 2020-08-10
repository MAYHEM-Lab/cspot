#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc-access.h"
#include "woofc-host.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft_client.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "w:r:d:i:"
char* Usage = "s_join_cluster -w node_woof -r raft_config -d dht_config\n\
\t-i to bind a certain IP\n";

char node_woof[DHT_NAME_LENGTH];

int main(int argc, char** argv) {
    char ip[256] = {0};
    char dht_config[256] = {0};
    char raft_config[256] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'w': {
            strncpy(node_woof, optarg, sizeof(node_woof));
            break;
        }
        case 'd': {
            strncpy(dht_config, optarg, sizeof(dht_config));
            break;
        }
        case 'r': {
            strncpy(raft_config, optarg, sizeof(raft_config));
            break;
        }
        case 'i': {
            strncpy(ip, optarg, sizeof(ip));
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (node_woof[0] == 0 || dht_config[0] == 0 || raft_config[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }
    WooFInit();

    FILE* fp = fopen(dht_config, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open dht_config file\n");
        exit(1);
    }
    int stabilize_freq = 0;
    int chk_predecessor_freq = 0;
    int fix_finger_freq = 0;
    int update_leader_freq = 0;
    int daemon_wakeup_freq = 0;
    if (read_dht_config(
            fp, &stabilize_freq, &chk_predecessor_freq, &fix_finger_freq, &update_leader_freq, &daemon_wakeup_freq) <
        0) {
        fprintf(stderr, "failed to read dht_config file\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);
    fp = fopen(raft_config, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open raft_config file\n");
        exit(1);
    }
    char name[DHT_NAME_LENGTH];
    int num_replica;
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    if (read_raft_config(fp, name, &num_replica, replicas) < 0) {
        fprintf(stderr, "failed to read raft_config file\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    char woof_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(woof_namespace);
    if (ip[0] == 0) {
        if (WooFLocalIP(ip, sizeof(ip)) < 0) {
            fprintf(stderr, "didn't specify IP to bind and couldn't find local IP\n");
            exit(1);
        }
    }
    char woof_name[DHT_NAME_LENGTH] = {0};
    sprintf(woof_name, "woof://%s%s", ip, woof_namespace);

#ifdef USE_RAFT
    if (raft_is_leader()) {
        if (dht_join_cluster(node_woof,
                             woof_name,
                             name,
                             replicas,
                             stabilize_freq,
                             chk_predecessor_freq,
                             fix_finger_freq,
                             update_leader_freq,
                             daemon_wakeup_freq) < 0) {
            fprintf(stderr, "failed to join cluster: %s\n", dht_error_msg);
            exit(1);
        }
    } else {
        unsigned char node_hash[SHA_DIGEST_LENGTH];
        dht_hash(node_hash, name);

        if (dht_init(node_hash, name, woof_name, replicas) < 0) {
            fprintf(stderr, "failed to initialize DHT: %s\n", dht_error_msg);
            exit(1);
        }
        if (monitor_create(DHT_MONITOR_NAME) < 0) {
            fprintf(stderr, "failed to create and start the handler monitor\n");
            return -1;
        }
        if (dht_start_daemon(
                stabilize_freq, chk_predecessor_freq, fix_finger_freq, update_leader_freq, daemon_wakeup_freq) < 0) {
            fprintf(stderr, "failed to start the daemon\n");
            exit(1);
        }
    }
#else
    if (dht_join_cluster(node_woof,
                         woof_name,
                         name,
                         replicas,
                         stabilize_freq,
                         chk_predecessor_freq,
                         fix_finger_freq,
                         update_leader_freq,
                         daemon_wakeup_freq) < 0) {
        fprintf(stderr, "failed to join cluster: %s\n", dht_error_msg);
        exit(1);
    }
#endif

    return 0;
}
