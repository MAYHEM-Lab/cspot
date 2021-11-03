#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <cstdint>
#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>

using namespace std;

void restore_model(REGRESSOR_MODEL* model, mlpack::regression::LinearRegression* regressor) {
    arma::vec& p = regressor->Parameters();
    p.set_size(model->n_elem);
    for (int i = 0; i < model->n_elem; ++i) {
        p(i) = model->p_vec[i];
    }
    double& l = regressor->Lambda();
    l = model->lambda;
}

void cpu_topic_name(int i, char* topic_name) {
    switch (i) {
    case 0:
        sprintf(topic_name, "%s%s", TOPIC_CPU_1, TOPIC_SMOOTH_SUFFIX);
        break;
    case 1:
        sprintf(topic_name, "%s%s", TOPIC_CPU_2, TOPIC_SMOOTH_SUFFIX);
        break;
    case 2:
        sprintf(topic_name, "%s%s", TOPIC_CPU_3, TOPIC_SMOOTH_SUFFIX);
        break;
    case 3:
        sprintf(topic_name, "%s%s", TOPIC_CPU_4, TOPIC_SMOOTH_SUFFIX);
        break;
    default:
        break;
    }
}

void refresh_model(REGRESSOR_MODEL* model) {
    if (dht_publish((char*)TOPIC_REGRESSOR_MODEL, model, sizeof(REGRESSOR_MODEL)) < 0) {
        cerr << "[predict] failed to refresh model: " << dht_error_msg << endl;
        exit(1);
    }
}

extern "C" int h_predict(char* topic_name, unsigned long seq_no, void* ptr) {
    DATA_ELEMENT* el = (DATA_ELEMENT*)ptr;

    uint64_t triggered = get_milliseconds();
    cout << "[predict] latency trigger: " << triggered - el->publish_ts << endl;
    unsigned long latest_model = dht_latest_index((char*)TOPIC_REGRESSOR_MODEL);
    if (WooFInvalid(latest_model)) {
        cerr << "[predict] failed to get the latest index of " << TOPIC_REGRESSOR_MODEL << ": " << dht_error_msg
             << endl;
        exit(1);
    } else if (latest_model == 0) {
        // cout << "[predict] no model has been trained yet" << endl;
        return 1;
    }

    RAFT_DATA_TYPE data = {0};
    if (dht_get((char*)TOPIC_REGRESSOR_MODEL, &data, latest_model) < 0) {
        cerr << "[predict] failed to get the latest model: " << dht_error_msg << endl;
        exit(1);
    }
    REGRESSOR_MODEL* model = (REGRESSOR_MODEL*)&data;
    if (seq_no % 1024 == 0) {
        refresh_model(model); // refresh model to avoid log rolled over
    }
    mlpack::regression::LinearRegression regressor;
    restore_model(model, &regressor);

    double cpu_reading[TOPIC_NUMBER] = {0};
    unsigned long cpu_index[TOPIC_NUMBER] = {0};
    for (int i = 0; i < TOPIC_NUMBER; ++i) {
        char cpu_topic[DHT_NAME_LENGTH] = {0};
        cpu_topic_name(i, cpu_topic);
        if (strcmp(cpu_topic, topic_name) == 0) {
            cpu_reading[i] = el->val;
            continue;
        }
        cpu_index[i] = dht_latest_index(cpu_topic);
        if (WooFInvalid(cpu_index[i])) {
            cerr << "[predict] failed to get the latest index of " << cpu_topic << ": " << dht_error_msg << endl;
            exit(1);
        }
        while (cpu_index[i] > 0) {
            if (dht_get(cpu_topic, &data, cpu_index[i]) < 0) {
                cerr << "[predict] failed to get data from " << cpu_topic << "[" << cpu_index[i]
                     << "]: " << dht_error_msg << endl;
                exit(1);
            }
            DATA_ELEMENT* cpu_data = (DATA_ELEMENT*)&data;
            if (cpu_data->ts == el->ts) {
                cpu_reading[i] = cpu_data->val;
                break;
            } else if (cpu_data->ts < el->ts) {
                break;
            }
            --cpu_index[i];
        }
        if (cpu_reading[i] == 0) {
            // cout << "[predict] " << cpu_topic << " data at " << el->ts << " not found" << endl;
            return 1;
        }
    }

    arma::mat p(TOPIC_NUMBER, 1);
    for (int i = 0; i < TOPIC_NUMBER; ++i) {
        p(i, 0) = cpu_reading[i];
    }
    // cout << "[predict] input: " << p(0) << " " << p(1) << " " << p(2) << " " << p(3) << endl;
    arma::rowvec res;
    regressor.Predict(p, res);

    double truth = 0;
    char smooth_dht[DHT_NAME_LENGTH] = {0};
    sprintf(smooth_dht, "%s%s", TOPIC_DHT_1, TOPIC_SMOOTH_SUFFIX);
    unsigned long dht_index = dht_latest_index(smooth_dht);
    if (WooFInvalid(dht_index)) {
        cerr << "[predict] failed to get the latest index of " << smooth_dht << ": " << dht_error_msg << endl;
        exit(1);
    }
    while (dht_index > 0) {
        if (dht_get(smooth_dht, &data, dht_index) < 0) {
            cerr << "[predict] failed to get data from " << smooth_dht << "[" << dht_index << "]: " << dht_error_msg
                 << endl;
            exit(1);
        }
        DATA_ELEMENT* dht_data = (DATA_ELEMENT*)&data;
        if (dht_data->ts == el->ts) {
            truth = dht_data->val;
            break;
        } else if (dht_data->ts < el->ts) {
            break;
        }
        --dht_index;
    }
    if (truth == 0) {
        cout << "[predict] " << el->ts << " prediction: " << res(0) << endl;
    } else {
        cout << "[predict] " << el->ts << " prediction: " << res(0) << ", truth: " << truth
             << ", error: " << abs(truth - res(0)) << endl;
    }

    DATA_ELEMENT predict = {0};
    predict.val = res(0);
    predict.ts = el->ts;
    predict.publish_ts = el->publish_ts;
    cout << "[predict] latency regress: " << get_milliseconds() - triggered << endl;
    if (dht_publish((char*)TOPIC_PREDICT, &predict, sizeof(DATA_ELEMENT)) < 0) {
        cerr << "[predict] failed to publish to " << TOPIC_PREDICT << ": " << dht_error_msg << endl;
        exit(1);
    }

    if (truth != 0) {
        DATA_ELEMENT error = {0};
        error.val = abs(truth - res(0));
        error.ts = el->ts;
        error.publish_ts = el->publish_ts;
        if (dht_publish((char*)TOPIC_ERROR, &error, sizeof(DATA_ELEMENT)) < 0) {
            cerr << "[predict] failed to publish to " << TOPIC_ERROR << ": " << dht_error_msg << endl;
            exit(1);
        }
    }

    return 1;
}
