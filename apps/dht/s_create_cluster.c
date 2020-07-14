#include "dht.h"
#include "dht_utils.h"
#include "woofc-access.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "f:i:"
char* Usage = "s_create_cluster -f config\n\
\t-i to bind a certain IP\n";

int main(int argc, char** argv) {
    char ip[256];
    char config[256];
    memset(ip, 0, sizeof(ip));
    memset(config, 0, sizeof(config));

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'f': {
            strncpy(config, optarg, sizeof(config));
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

    if (dht_create_cluster(woof_name, name, replicas) < 0) {
        fprintf(stderr, "failed to initialize the cluster: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
