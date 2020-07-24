#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern char WooF_dir[2048];

int h_register_topic(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_REGISTER_TOPIC_ARG* arg = (DHT_REGISTER_TOPIC_ARG*)ptr;

    log_set_tag("h_register_topic");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    char registration_woof[DHT_NAME_LENGTH] = {0};
    sprintf(registration_woof, "%s_%s", arg->topic_name, DHT_TOPIC_REGISTRATION_WOOF);
    if (!WooFExist(registration_woof)) {
        if (WooFCreate(registration_woof, sizeof(DHT_TOPIC_REGISTRY), DHT_HISTORY_LENGTH_SHORT) < 0) {
            log_error("failed to create woof %s", registration_woof);
            exit(1);
        }
        log_info("created registration woof %s", registration_woof);
        char woof_file[DHT_NAME_LENGTH] = {0};
        sprintf(woof_file, "%s/%s", WooF_dir, registration_woof);
        if (chmod(woof_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
            log_error("failed to change file %s's mode to 0666", registration_woof);
        }
    }

    DHT_TOPIC_REGISTRY topic_registry = {0};
    strcpy(topic_registry.topic_name, arg->topic_name);
#ifdef USE_RAFT
    memcpy(topic_registry.topic_replicas, arg->topic_replicas, sizeof(topic_registry.topic_replicas));
#else
    memcpy(topic_registry.topic_namespace, arg->topic_namespace, sizeof(topic_registry.topic_namespace));
#endif
    unsigned long seq = WooFPut(registration_woof, NULL, &topic_registry);
    if (WooFInvalid(seq)) {
        log_error("failed to register topic");
        exit(1);
    }
#ifdef USE_RAFT
    log_info("registered topic %s/%s", topic_registry.topic_replicas[0], topic_registry.topic_name);
#else
    log_info("registered topic %s/%s", topic_registry.topic_namespace, topic_registry.topic_name);
#endif

    char subscription_woof[DHT_NAME_LENGTH] = {0};
    sprintf(subscription_woof, "%s_%s", arg->topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
    if (!WooFExist(subscription_woof)) {
        if (WooFCreate(subscription_woof, sizeof(DHT_SUBSCRIPTION_LIST), DHT_HISTORY_LENGTH_SHORT) < 0) {
            log_error("failed to create %s", subscription_woof);
            exit(1);
        }
        log_info("created subscription woof %s", subscription_woof);
        char woof_file[DHT_NAME_LENGTH] = {0};
        sprintf(woof_file, "%s/%s", WooF_dir, subscription_woof);
        if (chmod(woof_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
            log_error("failed to change file %s's mode to 0666", woof_file);
        }
        DHT_SUBSCRIPTION_LIST list = {0};
        seq = WooFPut(subscription_woof, NULL, &list);
        if (WooFInvalid(seq)) {
            log_error("failed to initialize subscription list %s", subscription_woof);
            exit(1);
        }
    }

    return 1;
}
