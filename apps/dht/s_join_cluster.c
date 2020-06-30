#include "dht.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft_client.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "w:f:"
char* Usage = "s_join_cluster -w node_woof -f config\n";

char node_woof[DHT_NAME_LENGTH];

int main(int argc, char** argv) {
    char config[256];

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'w': {
            strncpy(node_woof, optarg, sizeof(node_woof));
            break;
        }
        case 'f': {
            strncpy(config, optarg, sizeof(config));
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (node_woof[0] == 0 || config[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }
    WooFInit();

    FILE* fp = fopen(config, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open config file\n");
        exit(1);
    }
    char name[DHT_NAME_LENGTH];
    int num_replica;
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    if (read_config(fp, name, &num_replica, replicas) < 0) {
        fprintf(stderr, "failed to read config file\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    char woof_name[DHT_NAME_LENGTH];
    if (node_woof_name(woof_name) < 0) {
        fprintf(stderr, "failed to get local node's woof name: %s\n", dht_error_msg);
        exit(1);
    }

#ifdef USE_RAFT
    if (raft_is_leader()) {
        if (dht_join_cluster(node_woof, woof_name, name, replicas) < 0) {
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
        if (dht_start_daemon() < 0) {
            fprintf(stderr, "failed to start the daemon\n");
            exit(1);
        }
    }
#else
    if (dht_join_cluster(node_woof, woof_name, name, replicas) < 0) {
        fprintf(stderr, "failed to join cluster: %s\n", dht_error_msg);
        exit(1);
    }
#endif

    return 0;
}
