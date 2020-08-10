#ifndef USE_RAFT
#define USE_RAFT
#endif

#include "dht_client.h"
#include "multi_regression.h"

#include <algorithm>
#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>

using namespace std;

void store_model(REGRESSOR_MODEL* model, mlpack::regression::LinearRegression regressor) {
    arma::vec& p = regressor.Parameters();
    model->n_rows = p.n_rows;
    model->n_cols = p.n_cols;
    model->n_elem = p.n_elem;
    for (int i = 0; i < p.n_elem; ++i) {
        model->p_vec[i] = p(i);
    }
    model->lambda = regressor.Lambda();
}

// return -1 if all are within 1 minute, or return the index of largerst timestamp
int align(TEMPERATURE_ELEMENT temp[6]) {
    unsigned long diff = 60 * 1000;
    unsigned long min_ts = ULONG_MAX, max_ts = 0;
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

extern "C" int h_train(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    cout << "h_train triggered by " << topic_name << endl;

    char temp_topics[6][13] = {TOPIC_PIZERO02_CPU,
                               TOPIC_PIZERO04_CPU,
                               TOPIC_PIZERO05_CPU,
                               TOPIC_PIZERO06_CPU,
                               TOPIC_WU_STATION,
                               TOPIC_PIZERO02_DHT};

    double cpu[5][TRAINING_WINDOW] = {0};
    double dht[TRAINING_WINDOW] = {0};

    unsigned long seqno[6] = {0};
    for (int i = 0; i < 6; ++i) {
        seqno[i] = dht_topic_latest_seqno((char*)temp_topics[i]);
    }

    for (int i = 0; i < TRAINING_WINDOW; ++i) {
        TEMPERATURE_ELEMENT temp[6] = {0};
        for (int j = 0; j < 6; ++j) {
            if (dht_topic_is_empty(seqno[j])) {
                cout << "not enough data in " << temp_topics[j] << endl;
                cout << i << " samples collected" << endl;
                return 1;
            }
            if (dht_topic_get((char*)temp_topics[j], &temp[j], sizeof(TEMPERATURE_ELEMENT), seqno[j]) < 0) {
                cerr << "failed to get the temperature from " << temp_topics[j] << ": " << dht_error_msg << endl;
                exit(1);
            }
        }
        int largest_timestamp = -1;
        while ((largest_timestamp = align(temp)) != -1) {
            // cout << "the timestamp of " << temp_topics[largest_timestamp] << " " <<
            // temp[largest_timestamp].timestamp
            // << " is not within 10 seconds" << endl;
            --seqno[largest_timestamp];
            if (seqno[largest_timestamp] <= 1) { // when topic is initialized, there's a init mapping with seqno = 1
                cout << "not enough data in " << temp_topics[largest_timestamp] << endl;
                cout << i << " samples collected" << endl;
                return 1;
            }
            if (dht_topic_get((char*)temp_topics[largest_timestamp],
                              &temp[largest_timestamp],
                              sizeof(TEMPERATURE_ELEMENT),
                              seqno[largest_timestamp]) < 0) {
                cerr << "failed to get the temperature from " << temp_topics[largest_timestamp] << ": " << dht_error_msg
                     << endl;
                exit(1);
            }
        }
        for (int j = 0; j < 5; ++j) {
            cpu[j][i] = temp[j].temp;
        }
        dht[i] = temp[5].temp;
        for (int j = 0; j < 6; ++j) {
            --seqno[j];
        }
    }

    arma::mat pred(5, TRAINING_WINDOW);
    arma::rowvec resp(TRAINING_WINDOW);
    for (int i = 0; i < TRAINING_WINDOW; ++i) {
        for (int j = 0; j < 5; ++j) {
            pred(j, i) = cpu[j][i];
            resp(i) = dht[i];
        }
    }

    mlpack::regression::LinearRegression regressor;
    regressor.Train(pred, resp);
    cout << "trained regressor parameters: " << regressor.Parameters() << endl;

    REGRESSOR_MODEL model = {0};
    store_model(&model, regressor);
    if (dht_publish((char*)TOPIC_REGRESSOR_MODEL, &model, sizeof(REGRESSOR_MODEL)) < 0) {
        cerr << "failed to publish trained model: " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
