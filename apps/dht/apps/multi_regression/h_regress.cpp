#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>
#include <stdint.h>

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

extern "C" int h_regress(char* topic_name, unsigned long seq_no, void* ptr) {
    // cout << "h_regress triggered by " << topic_name << endl;
    TEMPERATURE_ELEMENT* el = (TEMPERATURE_ELEMENT*)ptr;

    char temp_topics[5][13] = {
        TOPIC_PIZERO02_CPU, TOPIC_PIZERO04_CPU, TOPIC_PIZERO05_CPU, TOPIC_PIZERO06_CPU, TOPIC_WU_STATION};

    unsigned long latest_model = dht_latest_index((char*)TOPIC_REGRESSOR_MODEL);
    if (WooFInvalid(latest_model)) {
        cout << "[regress] failed to get the latest index of " << TOPIC_REGRESSOR_MODEL << ": " << dht_error_msg
             << endl;
        return 1;
    } else if (latest_model == 0) {
        cout << "[regress] no model has been trained yet" << endl;
        return 1;
    }

    RAFT_DATA_TYPE data = {0};
    REGRESSOR_MODEL model = {0};
    if (dht_get((char*)TOPIC_REGRESSOR_MODEL, &data, latest_model) < 0) {
        cerr << "[regress] failed to get the latest model: " << dht_error_msg << endl;
        exit(1);
    }
    memcpy(&model, &data, sizeof(REGRESSOR_MODEL));
    mlpack::regression::LinearRegression regressor;
    restore_model(model, &regressor);

    arma::mat p(5, 1);
    TEMPERATURE_ELEMENT temp[5] = {0};
    unsigned long seqno[5] = {0};
    for (int i = 0; i < 5; ++i) {
        seqno[i] = dht_latest_index((char*)temp_topics[i]);
        if (WooFInvalid(seqno[i])) {
            cout << "[regress] failed to get the latest index of " << temp_topics[i] << ": " << dht_error_msg << endl;
            return 1;
        } else if (seqno[i] == 0) {
            cout << "[regress] no data in " << temp_topics[i] << endl;
            return 1;
        }
        if (dht_get((char*)temp_topics[i], &data, seqno[i]) < 0) {
            cerr << "[regress] failed to get the temperature from " << temp_topics[i] << ": " << dht_error_msg << endl;
            exit(1);
        }
        memcpy(&temp[i], &data, sizeof(TEMPERATURE_ELEMENT));
        p(i, 0) = temp[i].temp;
    }

    // cout << "input: " << p(0) << " " << p(1) << " " << p(2) << " " << p(3) << " " << p(4) << endl;
    arma::rowvec res;
    regressor.Predict(p, res);

    unsigned long dht_seqno = dht_latest_index((char*)TOPIC_PIZERO02_DHT);
    if (WooFInvalid(dht_seqno)) {
        cout << "[regress] failed to get the latest index of " << TOPIC_PIZERO02_DHT << ": " << dht_error_msg << endl;
        return 1;
    } else if (dht_seqno == 0) {
        cout << "[regress] no data in " << TOPIC_PIZERO02_DHT << endl;
        return 1;
    }
    TEMPERATURE_ELEMENT dht_reading = {0};
    while (dht_seqno > 1) {
        if (dht_get((char*)TOPIC_PIZERO02_DHT, &data, dht_seqno) < 0) {
            cerr << "[regress] failed to get the temperature from " << TOPIC_PIZERO02_DHT << ": " << dht_error_msg
                 << endl;
            exit(1);
        }
        memcpy(&dht_reading, &data, sizeof(TEMPERATURE_ELEMENT));
        unsigned long pred_ts = el->timestamp / (300 * 1000);
        unsigned long dht_ts = dht_reading.timestamp / (300 * 1000);
        if (dht_ts == pred_ts) {
            cout << "[regress] predict: " << res(0) << ", truth: " << dht_reading.temp
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
    if (dht_publish((char*)TOPIC_PIZERO02_PREDICT, &predict, sizeof(TEMPERATURE_ELEMENT)) < 0) {
        cerr << "[regress] failed to publish the predict temperature: " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
