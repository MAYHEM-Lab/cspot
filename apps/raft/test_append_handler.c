#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "f:t"
char* Usage = "test_append_handler -f config_file -t timeout\n";

typedef struct test_stc {
    char msg[256];
} TEST_EL;

int main(int argc, char** argv) {
    char config_file[256] = "";
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'f': {
            strncpy(config_file, optarg, sizeof(config_file));
            break;
        }
        case 't': {
            timeout = (int)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
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
        fprintf(stderr, "can't read config file\n");
        exit(1);
    }
    int timeout_min, timeout_max;
    char name[RAFT_NAME_LENGTH];
    int members;
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
    if (read_config(fp, &timeout_min, &timeout_max, name, &members, member_woofs) < 0) {
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

    char current_leader[RAFT_NAME_LENGTH];
    if (raft_current_leader(raft_client_leader, current_leader) < 0) {
        fprintf(stderr, "couldn't get the current leader\n");
        exit(1);
    }
    raft_set_client_leader(current_leader);

    TEST_EL test_el = {0};
    sprintf(test_el.msg, "test_append_handler");
    unsigned long index = raft_put_handler("test_raft_handler", &test_el, sizeof(TEST_EL), 0, 0);
    while (index == RAFT_REDIRECTED) {
        index = raft_put_handler("test_raft_handler", &test_el, sizeof(TEST_EL), 0, 0);
    }
    if (raft_is_error(index)) {
        fprintf(stderr, "failed to put %s: %s\n", test_el.msg, raft_error_msg);
    } else {
        printf("put data: %s, index: %lu\n", test_el.msg, index);
    }

    return 0;
}
