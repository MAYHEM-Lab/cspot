#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "d:r:i:"
char* Usage = "s_create_cluster -d dht_config -r raft_config\n\
\t-i to bind a certain IP\n";

int main(int argc, char** argv) {
    char ip[256] = {0};
    char dht_config[256] = {0};
    char raft_config[256] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
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

    if (dht_config[0] == 0 || raft_config[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }
    WooFInit();

    FILE* fp = fopen(dht_config, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open dht_config file %s\n", dht_config);
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
        fprintf(stderr, "failed to read dht_config file %s: %s\n", dht_config, dht_error_msg);
        fclose(fp);
        exit(1);
    }
    fclose(fp);
    fp = fopen(raft_config, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open raft_config file %s\n", raft_config);
        exit(1);
    }
    char name[DHT_NAME_LENGTH];
    int num_replica;
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    if (read_raft_config(fp, name, &num_replica, replicas) < 0) {
        fprintf(stderr, "failed to read config file %s: %s\n", raft_config, dht_error_msg);
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

    if (dht_create_cluster(woof_name,
                           name,
                           replicas,
                           stabilize_freq,
                           chk_predecessor_freq,
                           fix_finger_freq,
                           update_leader_freq,
                           daemon_wakeup_freq) < 0) {
        fprintf(stderr, "failed to initialize the cluster: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
