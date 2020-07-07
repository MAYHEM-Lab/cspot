#include "dht.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft_client.h"
#endif

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    WooFInit();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }
    DHT_PREDECESSOR_INFO predecessor = {0};
    if (get_latest_predecessor_info(&predecessor) < 0) {
        log_error("couldn't get latest predecessor info: %s", dht_error_msg);
        exit(1);
    }
    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        exit(1);
    }

    int i;
    char woof_name[DHT_NAME_LENGTH];
    char hash[DHT_NAME_LENGTH];
    // node
#ifdef USE_RAFT
    int is_leader = raft_is_leader();
    if (is_leader < 0) {
        printf("leader: ???\n");
    } else if (is_leader) {
        printf("leader: yes\n");
    } else {
        printf("leader: no\n");
    }
#endif
    printf("node_name: %s\n", node.name);
    print_node_hash(hash, node.hash);
    printf("node_hash: %s\n", hash);
    printf("node_addr: %s\n", node.addr);
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (node.replicas[i][0] == 0) {
            break;
        }
        printf("node_replica: %s\n", node.replicas[i]);
    }
    printf("node_replica_id: %d\n", node.replica_id);
    printf("node_leader_id: %d\n", node.leader_id);

    // predecessor
    print_node_hash(hash, predecessor.hash);
    printf("predecessor_hash: %s\n", hash);
    if (is_empty(predecessor.hash)) {
        printf("predecessor_addr: nil\n");
    } else {
        print_node_hash(woof_name + strlen(woof_name), predecessor.hash);
        printf("predecessor_addr: %s\n", predecessor_addr(&predecessor));
        for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
            if (predecessor.replicas[i][0] == 0) {
                break;
            }
            printf("predecessor_replica: %s\n", predecessor.replicas[i]);
        }
    }

    // successor
    int j;
    for (j = 0; j < DHT_SUCCESSOR_LIST_R; ++j) {
        print_node_hash(hash, successor.hash[j]);
        printf("successor_hash %d: %s\n", j, hash);
        if (is_empty(successor.hash[j])) {
            printf("successor_addr %d: nil\n", j);
        } else {
            print_node_hash(woof_name + strlen(woof_name), successor.hash[j]);
            printf("successor_addr %d: %s\n", j, successor_addr(&successor, j));
            for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
                if (successor.replicas[i][0] == 0) {
                    break;
                }
                printf("successor_replica: %s\n", successor.replicas[i]);
            }
        }
    }

    // finger
    int all_filled = 1;
    for (j = 1; j <= SHA_DIGEST_LENGTH * 8; ++j) {
        DHT_FINGER_INFO finger = {0};
        if (get_latest_finger_info(j, &finger) < 0) {
            fprintf(stderr, "failed to get finger[%d]'s info: %s", j, dht_error_msg);
            exit(1);
        }
        print_node_hash(hash, finger.hash);
        printf("finger_hash %d: %s\n", j, hash);
        if (is_empty(finger.hash)) {
            all_filled = 0;
            printf("finger_addr %d: nil\n", j);
        } else {
            printf("finger_addr %d: %s\n", j, finger_addr(&finger));
        }
    }
    if (all_filled) {
        printf("all fingers filled: yes\n");
    } else {
        printf("all fingers filled: no\n");
    }
    return 0;
}
