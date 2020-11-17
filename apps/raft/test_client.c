#include "raft.h"
#include "raft_client.h"
#include "raft_utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "f:c:t:s"
char* Usage = "test_client -f config_file -c count\n\
-s for synchronously put, -t timeout\n";

int main(int argc, char** argv) {
    char config_file[256] = "";
    int count = 10;
    int sync = 0;
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'f': {
            strncpy(config_file, optarg, sizeof(config_file));
            break;
        }
        case 'c': {
            count = (int)strtoul(optarg, NULL, 0);
            break;
        }
        case 's': {
            sync = 1;
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
    int timeout_min, timeout_max, replicate_delay;
    int members;
    char name[RAFT_NAME_LENGTH];
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
    if (read_config(fp, &timeout_min, &timeout_max, &replicate_delay, name, &members, member_woofs) < 0) {
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

    uint64_t begin, finish;
    if (sync) {
        int i;
        for (i = 0; i < count; ++i) {
            RAFT_DATA_TYPE data = {0};
            sprintf(data.val, "sync_%d", i);
            begin = get_milliseconds();
            unsigned long index = raft_sync_put(&data, timeout);
            finish = get_milliseconds();
            while (index == RAFT_REDIRECTED) {
                begin = get_milliseconds();
                index = raft_sync_put(&data, timeout);
                finish = get_milliseconds();
            }
            if (raft_is_error(index)) {
                fprintf(stderr, "failed to put %s: %s\n", data.val, raft_error_msg);
                fflush(stderr);
            } else {
                printf("put data[%d]: %s, index: %lu, %lu\n", i, data.val, index, finish - begin);
                fflush(stdout);
            }
        }
    } else {
        unsigned long seq[count];
        int i;
        for (i = 0; i < count; ++i) {
            RAFT_DATA_TYPE data = {0};
            sprintf(data.val, "async_%d", i);
            seq[i] = raft_async_put(&data);
            if (raft_is_error(seq[i])) {
                fprintf(stderr, "failed to put %s: %s\n", data.val, raft_error_msg);
                fflush(stderr);
            } else {
                printf("put data[%d]: %s, client_request_seqno: %lu\n", i, data.val, seq[i]);
                fflush(stdout);
            }
        }
    }

    return 0;
}
