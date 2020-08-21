#ifndef USE_RAFT
#define USE_RAFT
#endif

#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <algorithm>
#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>
#include <stdint.h>

#define TIMEOUT 3000
#define TRAINING_WINDOW (12 * 60 * 60 * 1000)
#define MAX_SAMPLES (TRAINING_WINDOW / 5 / 60 / 1000)

using namespace std;

void store_model(REGRESSOR_MODEL* model, mlpack::regression::LinearRegression regressor) {
    arma::vec& p = regressor.Parameters();
    model->n_rows = (int32_t)p.n_rows;
    model->n_cols = (int32_t)p.n_cols;
    model->n_elem = (int32_t)p.n_elem;
    for (int i = 0; i < p.n_elem; ++i) {
        model->p_vec[i] = p(i);
    }
    model->lambda = regressor.Lambda();
}

// return -1 if all are within 1 minute, or return the index of largerst timestamp
int align(TEMPERATURE_ELEMENT temp[6]) {
    uint64_t diff = 60 * 1000;
    uint64_t min_ts = ULONG_MAX, max_ts = 0;
    int max_index = -1;
    for (int i = 0; i < 6; ++i) {
        min_ts = min(min_ts, temp[i].timestamp);
        if (temp[i].timestamp > max_ts) {
            max_ts = temp[i].timestamp;
            max_index = i;
        }
    }
    if (max_ts - min_ts < diff) {
        return -1;
    }
    return max_index;
}

int get_topic_replica(char* topic_name, char* replica) {
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int node_leader;
    int hops;
    if (dht_find_node(topic_name, node_replicas, &node_leader, &hops, TIMEOUT) < 0) {
        fprintf(stderr, "failed to find node hosting the topic %s\n", topic_name);
        return -1;
    }
    char* node_addr = node_replicas[node_leader];
    char registration_woof[DHT_NAME_LENGTH] = {0};
    sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);
    DHT_TOPIC_REGISTRY topic_entry = {0};
    if (get_latest_element(registration_woof, &topic_entry) < 0) {
        fprintf(stderr, "failed to get topic registration info from %s: %s\n", registration_woof, dht_error_msg);
        return -1;
    }
    strcpy(replica, topic_entry.topic_replicas[0]);
    return 0;
}

extern "C" int h_train(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    // cout << "h_train triggered by " << topic_name << endl;
    TEMPERATURE_ELEMENT* el = (TEMPERATURE_ELEMENT*)ptr;
    int remain = (el->timestamp / 10 * 10) % TRAINING_WINDOW;
    if (remain == 0) {
        cout << "start training with one day window (" << MAX_SAMPLES << " samples)" << endl;
    } else {
        cout << ((TRAINING_WINDOW - remain) / 1000 / 60) << " minutes until next training" << endl;
        return 1;
    }

    char temp_topics[6][13] = {TOPIC_PIZERO02_CPU,
                               TOPIC_PIZERO04_CPU,
                               TOPIC_PIZERO05_CPU,
                               TOPIC_PIZERO06_CPU,
                               TOPIC_WU_STATION,
                               TOPIC_PIZERO02_DHT};
    char topic_replica[6][DHT_NAME_LENGTH] = {0};
    for (int i = 0; i < 6; ++i) {
        if (get_topic_replica(temp_topics[i], topic_replica[i]) < 0) {
            cerr << "failed to get the replica address hosting the topic " << temp_topics[i] << endl;
            exit(1);
        }
    }

    double cpu[5][MAX_SAMPLES] = {0};
    double dht[MAX_SAMPLES] = {0};

    unsigned long seqno[6] = {0};
    for (int i = 0; i < 6; ++i) {
        seqno[i] = dht_remote_topic_latest_seqno((char*)topic_replica[i], (char*)temp_topics[i]);
        if (WooFInvalid(seqno[i])) {
            cerr << "failed to get the latest seqno from topic " << temp_topics[i] << endl;
            exit(1);
        }
    }

    bool not_enough = false;
    int collected = 0;
    for (int i = 0; i < MAX_SAMPLES; ++i) {
        TEMPERATURE_ELEMENT temp[6] = {0};
        for (int j = 0; j < 6; ++j) {
            if (dht_topic_is_empty(seqno[j])) {
                // cout << "not enough data in " << temp_topics[j] << endl;
                // cout << collected << " samples collected" << endl;
                not_enough = true;
                break;
            }
            if (dht_remote_topic_get(
                    (char*)topic_replica[j], (char*)temp_topics[j], &temp[j], sizeof(TEMPERATURE_ELEMENT), seqno[j]) <
                0) {
                cerr << "failed to get the temperature from " << temp_topics[j] << ": " << dht_error_msg << seqno[j]
                     << endl;
                // cerr << collected << " samples collected" << endl;
                exit(1);
            }
        }
        int largest_timestamp = -1;
        while (!not_enough && (largest_timestamp = align(temp)) != -1) {
            // cout << "the timestamp of " << temp_topics[largest_timestamp] << " " <<
            // temp[largest_timestamp].timestamp
            // << " is not within 10 seconds" << endl;
            --seqno[largest_timestamp];
            if (dht_topic_is_empty(
                    seqno[largest_timestamp])) { // when topic is initialized, there's a init mapping with seqno = 1
                // cout << "not enough data in " << temp_topics[largest_timestamp] << endl;
                // cout << collected << " samples collected" << endl;
                not_enough = true;
                break;
            }
            uint64_t upper_ts = el->timestamp - TRAINING_WINDOW;
            int err = dht_remote_topic_get_range((char*)topic_replica[largest_timestamp],
                                                 (char*)temp_topics[largest_timestamp],
                                                 &temp[largest_timestamp],
                                                 sizeof(TEMPERATURE_ELEMENT),
                                                 seqno[largest_timestamp],
                                                 upper_ts,
                                                 0);
            if (err == -2) {
                // cout << collected << " samples collected for past one day" << endl;
                break;
            } else if (err < 0) {
                cerr << "failed to get the temperature from " << temp_topics[largest_timestamp] << ": " << dht_error_msg
                     << endl;
                // cerr << collected << " samples collected" << endl;
                exit(1);
            }
        }
        if (not_enough) {
            break;
        }
        for (int j = 0; j < 5; ++j) {
            cpu[j][i] = temp[j].temp;
        }
        dht[i] = temp[5].temp;
        for (int j = 0; j < 6; ++j) {
            --seqno[j];
        }
        collected = i + 1;
    }

    if (collected == 0) {
        cout << "no samples collected" << endl;
        return 1;
    }
    cout << "start training with " << collected << " samples" << endl;
    arma::mat pred(5, collected);
    arma::rowvec resp(collected);
    for (int i = 0; i < collected; ++i) {
        for (int j = 0; j < 5; ++j) {
            pred(j, i) = cpu[j][i];
            resp(i) = dht[i];
        }
    }

    mlpack::regression::LinearRegression regressor;
    regressor.Train(pred, resp);
    arma::vec& p = regressor.Parameters();
    cout << "trained regressor parameters: " << p(0) << " " << p(1) << " " << p(2) << " " << p(3) << " " << p(4) << " "
         << p(5) << endl;

    REGRESSOR_MODEL model = {0};
    store_model(&model, regressor);
    if (dht_publish((char*)TOPIC_REGRESSOR_MODEL, &model, sizeof(REGRESSOR_MODEL), TIMEOUT) < 0) {
        cerr << "failed to publish trained model: " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
