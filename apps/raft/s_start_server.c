#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc-access.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "f:oi:"
char* Usage = "s_start_server -f config_file\n\
\t-o as observer\n\
\t-i to bind a certain IP\n";

int main(int argc, char** argv) {
    char config_file[256] = {0};
    int observer = 0;
    char host_ip[256] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'f': {
            strncpy(config_file, optarg, sizeof(config_file));
            break;
        }
        case 'o': {
            observer = 1;
            break;
        }
        case 'i': {
            strncpy(host_ip, optarg, sizeof(host_ip));
            break;
        }
        default: {
            fprintf(stderr, "Unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (config_file[0] == 0) {
        fprintf(stderr, "Must specify config file\n");
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    FILE* fp = fopen(config_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Can't open config file\n");
        exit(1);
    }
    int timeout_min, timeout_max, replicate_delay;
    char name[RAFT_NAME_LENGTH];
    int members;
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
    if (read_config(fp, &timeout_min, &timeout_max, &replicate_delay, name, &members, member_woofs) < 0) {
        fprintf(stderr, "Can't read config file\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    WooFInit();

    if (host_ip[0] == 0) {
        if (WooFLocalIP(host_ip, sizeof(host_ip)) < 0) {
            fprintf(stderr, "didn't specify IP to bind and couldn't get local IP\n");
            exit(1);
        }
    }
    char woof_namespace[RAFT_NAME_LENGTH] = {0};
    node_woof_namespace(woof_namespace);
    char woof_name[RAFT_NAME_LENGTH] = {0};
    sprintf(woof_name, "woof://%s%s", host_ip, woof_namespace);

    if (raft_start_server(members, woof_name, member_woofs, observer, timeout_min, timeout_max) < 0) {
        fprintf(stderr, "Can't start server\n");
        exit(1);
    }

    return 0;
}
