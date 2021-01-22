#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <algorithm>
#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>
#include <stdint.h>

#define TRAINING_WINDOW (1 * 60 * 60 * 1000)
#define TRAINING_INTERVAL (5 * 60 * 1000)
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

extern "C" int h_train(char* topic_name, unsigned long seq_no, void* ptr) {
    // cout << "h_train triggered by " << topic_name << endl;
    TEMPERATURE_ELEMENT* el = (TEMPERATURE_ELEMENT*)ptr;
    int remain = (el->timestamp / 10 * 10) % TRAINING_INTERVAL;
    if (remain == 0) {
        cout << "[train] start collecting data for training" << endl;
    } else {
        cout << "[train] " << ((TRAINING_INTERVAL - remain) / 1000 / 60) << " minutes until next training" << endl;
        return 1;
    }

    char temp_topics[6][13] = {TOPIC_PIZERO02_CPU,
                               TOPIC_PIZERO04_CPU,
                               TOPIC_PIZERO05_CPU,
                               TOPIC_PIZERO06_CPU,
                               TOPIC_WU_STATION,
                               TOPIC_PIZERO02_DHT};

    double cpu[5][MAX_SAMPLES] = {0};
    double dht[MAX_SAMPLES] = {0};

    unsigned long seqno[6] = {0};
    for (int i = 0; i < 6; ++i) {
        seqno[i] = dht_latest_index((char*)temp_topics[i]);
        if (WooFInvalid(seqno[i])) {
            cerr << "[train] failed to get the latest seqno from topic " << temp_topics[i] << endl;
            exit(1);
        }
    }

    bool not_enough = false;
    int collected = 0;
    RAFT_DATA_TYPE data = {0};
    for (int i = 0; i < MAX_SAMPLES; ++i) {
        TEMPERATURE_ELEMENT temp[6] = {0};
        for (int j = 0; j < 6; ++j) {
            if (WooFInvalid(seqno[j])) {
                cout << "[train] failed to get the latest index of " << temp_topics[j] << ": " << dht_error_msg << endl;
                break;
            } else if (seqno[j] == 0) {
                cout << "[train] not enough data in " << temp_topics[j] << endl;
                cout << "[train] " << collected << " samples collected" << endl;
                not_enough = true;
                break;
            }
            if (dht_get((char*)temp_topics[j], &data, seqno[j]) < 0) {
                cerr << "[train]failed to get the temperature from " << temp_topics[j] << ": " << dht_error_msg
                     << seqno[j] << endl;
                cerr << "[train] " << collected << " samples collected" << endl;
                exit(1);
            }
            memcpy(&temp[j], &data, sizeof(TEMPERATURE_ELEMENT));
        }
        int largest_timestamp = -1;
        while (!not_enough && (largest_timestamp = align(temp)) != -1) {
            // cout << "[train] the timestamp of " << temp_topics[largest_timestamp] << " "
            //      << temp[largest_timestamp].timestamp << " is not within 10 seconds" << endl;
            --seqno[largest_timestamp];
            if (WooFInvalid(seqno[largest_timestamp])) {
                cout << "[train] failed to get the latest index of " << temp_topics[largest_timestamp] << ": "
                     << dht_error_msg << endl;
                break;
            } else if (seqno[largest_timestamp] == 0) {
                // cout << "[train] not enough data in " << temp_topics[largest_timestamp] << endl;
                // cout << "[train] " << collected << " samples collected" << endl;
                not_enough = true;
                break;
            }
            uint64_t upper_ts = el->timestamp - TRAINING_WINDOW;
            if (dht_get((char*)temp_topics[largest_timestamp], &data, seqno[largest_timestamp]) < 0) {
                cerr << "[train] failed to get the temperature from " << temp_topics[largest_timestamp] << ": "
                     << dht_error_msg << endl;
                cerr << "[train] " << collected << " samples collected" << endl;
                exit(1);
            }
            memcpy(&temp[largest_timestamp], &data, sizeof(TEMPERATURE_ELEMENT));
            if (temp[largest_timestamp].timestamp < upper_ts) {
                break;
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

    if (collected < MAX_SAMPLES) {
        cout << "[train] not enough samples collected (" << collected << ")" << endl;
        return 1;
    }
    cout << "[train] start training with " << collected << " samples" << endl;
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
    cout << "[train] trained regressor parameters: " << p(0) << " " << p(1) << " " << p(2) << " " << p(3) << " " << p(4)
         << " " << p(5) << endl;

    REGRESSOR_MODEL model = {0};
    store_model(&model, regressor);
    if (dht_publish((char*)TOPIC_REGRESSOR_MODEL, &model, sizeof(REGRESSOR_MODEL)) < 0) {
        cerr << "[train] failed to publish trained model: " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
