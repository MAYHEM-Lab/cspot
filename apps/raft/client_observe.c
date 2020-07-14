#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "f:t:i:"
char* Usage = "client_observe -f config -t timeout -i client_ip\n";

int get_result_delay = 20;

int main(int argc, char** argv) {
    char config[256] = {0};
    char client_ip[RAFT_NAME_LENGTH] = {0};
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'f': {
            strncpy(config, optarg, sizeof(config));
            break;
        }
        case 't': {
            timeout = (int)strtoul(optarg, NULL, 0);
            break;
        }
        case 'i': {
            strncpy(client_ip, optarg, sizeof(client_ip));
            break;
        }
        default: {
            fprintf(stderr, "Unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (config[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    FILE* fp = fopen(config, "r");
    if (fp == NULL) {
        fprintf(stderr, "can't read config file\n");
        exit(1);
    }
    char cluster_name[RAFT_NAME_LENGTH] = {0};
    int members;
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH] = {0};
    if (read_config(fp, cluster_name, &members, member_woofs) < 0) {
        fprintf(stderr, "failed to read the config file\n");
        fclose(fp);
        exit(1);
    }
    if (raft_init_client(members, member_woofs) < 0) {
        fprintf(stderr, "can't init client\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    fp = fopen(config, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open new config file %s\n", config);
        exit(1);
    }
    fclose(fp);

    if (client_ip[0] != 0) {
        if (WooFLocalIP(client_ip, sizeof(client_ip)) < 0) {
            fprintf(stderr, "didn't specify client IP and couldn't get local IP\n");
            exit(1);
        }
    }
    char observer_namespace[RAFT_NAME_LENGTH] = {0};
    node_woof_namespace(observer_namespace);
    char observer_woof[RAFT_NAME_LENGTH] = {0};
    sprintf(observer_woof, "woof://%s%s", client_ip, observer_namespace);

    int err = raft_observe(observer_woof, timeout);
    while (err == RAFT_REDIRECTED) {
        err = raft_observe(observer_woof, timeout);
    }

    return err;
}
