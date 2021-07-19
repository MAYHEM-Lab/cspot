#include "raft.h"
#include "raft_utils.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        fprintf(stderr, "failed to get the latest server state: %s\n", raft_error_msg);
        exit(1);
    }
    printf("woof_name: %s\n", server_state.woof_name);
    printf("current_term: %lu\n", server_state.current_term);
    printf("current_leader: %s\n", server_state.current_leader);
    switch (server_state.role) {
    case RAFT_LEADER: {
        printf("role: LEADER\n");
        break;
    }
    case RAFT_FOLLOWER: {
        printf("role: FOLLOWER\n");
        break;
    }
    case RAFT_CANDIDATE: {
        printf("role: CANDIDATE\n");
        break;
    }
    case RAFT_OBSERVER: {
        printf("role: OBSERVER\n");
        break;
    }
    case RAFT_SHUTDOWN: {
        printf("role: SHUTDOWN\n");
        break;
    }
    default: {
        printf("role: UNKNOWN\n");
        break;
    }
    }

    return 0;
}
