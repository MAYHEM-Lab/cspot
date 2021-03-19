#include "dht.h"
#include "dht_utils.h"
#include "raft_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    WooFInit();

    if (argc < 2) {
        fprintf(stderr, "./client_check_leader woof://.../namespace\n");
        exit(1);
    }

    RAFT_SERVER_STATE server_state = {0};
    char woof_name[DHT_NAME_LENGTH] = {0};
    sprintf(woof_name, "%s/%s", argv[1], RAFT_SERVER_STATE_WOOF);
    if (WooFGet(woof_name, &server_state, 0) < 0) {
        fprintf(stderr, "failed to get the current server_state\n");
        exit(1);
    }


    if (server_state.role == RAFT_LEADER) {
        printf("leader: yes\n");
    } else {
        printf("leader: no\n");
    }

    return 0;
}
