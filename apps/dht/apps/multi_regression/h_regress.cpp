#ifndef USE_RAFT
#define USE_RAFT
#endif

#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>
#include <stdint.h>

#define TIMEOUT 5000

using namespace std;

void restore_model(const REGRESSOR_MODEL model, mlpack::regression::LinearRegression* regressor) {
    arma::vec& p = regressor->Parameters();
    p.set_size(model.n_elem);
    for (int i = 0; i < model.n_elem; ++i) {
        p(i) = model.p_vec[i];
    }
    double& l = regressor->Lambda();
    l = model.lambda;
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

extern "C" int h_regress(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    // cout << "h_regress triggered by " << topic_name << endl;
    TEMPERATURE_ELEMENT* el = (TEMPERATURE_ELEMENT*)ptr;

    char temp_topics[5][13] = {
        TOPIC_PIZERO02_CPU, TOPIC_PIZERO04_CPU, TOPIC_PIZERO05_CPU, TOPIC_PIZERO06_CPU, TOPIC_WU_STATION};

    unsigned long latest_model = dht_topic_latest_seqno((char*)TOPIC_REGRESSOR_MODEL, TIMEOUT);
    if (dht_topic_is_empty(latest_model)) {
        cout << "no model has been trained yet" << endl;
        return 1;
    }

    REGRESSOR_MODEL model = {0};
    if (dht_topic_get((char*)TOPIC_REGRESSOR_MODEL, &model, sizeof(REGRESSOR_MODEL), latest_model, TIMEOUT) < 0) {
        cerr << "failed to get the latest model: " << dht_error_msg << endl;
        exit(1);
    }
    mlpack::regression::LinearRegression regressor;
    restore_model(model, &regressor);

    arma::mat p(5, 1);
    TEMPERATURE_ELEMENT temp[5] = {0};
    unsigned long seqno[5] = {0};
    for (int i = 0; i < 5; ++i) {
        seqno[i] = dht_topic_latest_seqno((char*)temp_topics[i], TIMEOUT);
        if (dht_topic_is_empty(seqno[i])) {
            cout << "no data in " << temp_topics[i] << endl;
            return 1;
        }
        if (dht_topic_get((char*)temp_topics[i], &temp[i], sizeof(TEMPERATURE_ELEMENT), seqno[i], TIMEOUT) < 0) {
            cerr << "failed to get the temperature from " << temp_topics[i] << ": " << dht_error_msg << endl;
            exit(1);
        }
        p(i, 0) = temp[i].temp;
    }

    // cout << "input: " << p(0) << " " << p(1) << " " << p(2) << " " << p(3) << " " << p(4) << endl;
    arma::rowvec res;
    regressor.Predict(p, res);

    unsigned long dht_seqno = dht_topic_latest_seqno((char*)TOPIC_PIZERO02_DHT, TIMEOUT);
    if (dht_topic_is_empty(dht_seqno)) {
        cout << "no data in " << TOPIC_PIZERO02_DHT << endl;
        return 1;
    }
    TEMPERATURE_ELEMENT dht_reading = {0};
    while (dht_seqno > 1) {
        if (dht_topic_get((char*)TOPIC_PIZERO02_DHT, &dht_reading, sizeof(TEMPERATURE_ELEMENT), dht_seqno, TIMEOUT) <
            0) {
            cerr << "failed to get the temperature from " << TOPIC_PIZERO02_DHT << ": " << dht_error_msg << endl;
            exit(1);
        }
        unsigned long pred_ts = el->timestamp / (300 * 1000);
        unsigned long dht_ts = dht_reading.timestamp / (300 * 1000);
        if (dht_ts == pred_ts) {
            cout << "predict: " << res(0) << ", truth: " << dht_reading.temp
                 << ", error: " << abs(res(0) - dht_reading.temp) << endl;
            break;
        } else if (dht_ts > pred_ts) {
            --dht_seqno;
        } else {
            break;
        }
    }

    TEMPERATURE_ELEMENT predict = {0};
    predict.temp = res(0);
    predict.timestamp = el->timestamp;
    if (dht_publish((char*)TOPIC_PIZERO02_PREDICT, &predict, sizeof(TEMPERATURE_ELEMENT), TIMEOUT) < 0) {
        cerr << "failed to publish the predict temperature: " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
