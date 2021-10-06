#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "o:f:t:"
char* Usage = "client_reconfig -o old_config -f new_config -t timeout\n";

int get_result_delay = 20;

int main(int argc, char** argv) {
    char old_config[256] = {0};
    char new_config[256] = {0};
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'o': {
            strncpy(old_config, optarg, sizeof(old_config));
            break;
        }
        case 'f': {
            strncpy(new_config, optarg, sizeof(new_config));
            break;
        }
        case 't': {
            timeout = (int)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "Unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (old_config[0] == 0 || new_config[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    // FILE* fp = fopen(old_config, "r");
    // if (fp == NULL) {
    //     fprintf(stderr, "can't read config file\n");
    //     exit(1);
    // }
    // int timeout_min, timeout_max, replicate_delay;
    // char name[RAFT_NAME_LENGTH];
    // int members;
    // char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
    // if (read_config(fp, &timeout_min, &timeout_max, &replicate_delay, name, &members, member_woofs) < 0) {
    //     fprintf(stderr, "failed to read the config file\n");
    //     fclose(fp);
    //     exit(1);
    // }
    // if (raft_init_client(members, member_woofs) < 0) {
    //     fprintf(stderr, "can't init client\n");
    //     fclose(fp);
    //     exit(1);
    // }
    // fclose(fp);

    // fp = fopen(new_config, "r");
    // if (fp == NULL) {
    //     fprintf(stderr, "failed to open new config file %s\n", new_config);
    //     exit(1);
    // }
    // if (read_config(fp, &timeout_min, &timeout_max, &replicate_delay, name, &members, member_woofs) < 0) {
    //     fprintf(stderr, "failed to read the config file\n");
    //     fclose(fp);
    //     exit(1);
    // }
    // fclose(fp);
    // int err = raft_config_change(members, member_woofs, timeout);
    // while (err == RAFT_REDIRECTED) {
    //     err = raft_config_change(members, member_woofs, timeout);
    // }

    // return err;

    printf("not implemented yet");
    return 0;
}
