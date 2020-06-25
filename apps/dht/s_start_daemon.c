#include "dht.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "f:"
char* Usage = "s_start_daemon -f config\n";

int main(int argc, char** argv) {
    char config[256];

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
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

    if (config[0] == 0) {
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

    unsigned char node_hash[SHA_DIGEST_LENGTH];
    dht_hash(node_hash, name);

    if (dht_init(node_hash, name, woof_name, replicas) < 0) {
        sprintf(dht_error_msg, "failed to initialize DHT: %s", dht_error_msg);
        return -1;
    }

    if (dht_start_daemon() < 0) {
        fprintf(stderr, "failed to start the daemon");
        exit(1);
    }

    return 0;
}
