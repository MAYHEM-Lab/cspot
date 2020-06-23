#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "f:o"
char* Usage = "s_start_server -f config_file\n\
-o as observer\n";

int main(int argc, char** argv) {
    char config_file[256];
    int observer = 0;

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
    char name[RAFT_NAME_LENGTH];
    int members;
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
    if (read_config(fp, name, &members, member_woofs) < 0) {
        fprintf(stderr, "Can't read config file\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    WooFInit();
    if (raft_start_server(members, member_woofs, observer) < 0) {
        fprintf(stderr, "Can't start server\n");
        exit(1);
    }

    return 0;
}
