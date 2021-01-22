#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"
#include "woofc-access.h"

int h_publish(WOOF* wf, unsigned long seq_no, void* ptr) {
    PUBLISH_ARG* arg = (PUBLISH_ARG*)ptr;

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        fprintf(stderr, "failed to get the latest dht node info: %s\n", dht_error_msg);
        exit(1);
    }
    char ip[DHT_NAME_LENGTH];
    if (WooFIPAddrFromURI(node.addr, ip, sizeof(ip)) < 0) {
        fprintf(stderr, "failed to extract IP from node address: %s\n", node.addr);
    }
    int port;
    if (WooFPortFromURI(node.addr, &port) < 0) {
        fprintf(stderr, "failed to extract port from node address: %s\n", node.addr);
    }
    char client_addr[DHT_NAME_LENGTH];
    sprintf(client_addr, "%s:%d", ip, port);
    dht_set_client_ip(client_addr);

    unsigned long index = dht_publish(arg->topic, &arg->val, sizeof(TEMPERATURE_ELEMENT));
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s: %s\n", arg->topic, dht_error_msg);
        exit(1);
    }
    printf("published to %s\n", arg->topic);
    return 1;
}
