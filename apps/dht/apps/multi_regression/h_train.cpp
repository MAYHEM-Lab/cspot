#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <algorithm>
#include <iostream>
#include <mlpack/core.hpp>
#include <mlpack/methods/hmm/hmm.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>
#include <stdint.h>

#define TRAINING_WINDOW (72 * 60 * 60 * 1000)
#define MODEL_EXPIRATION (72 * 60 * 60 * 1000)
#define MAX_SAMPLES (TRAINING_WINDOW / 5 / 60 / 1000)
#define MIN_SAMPLES (24 * 60 / 5) // one day 288

using namespace std;

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

extern "C" int h_train(char* topic_name, unsigned long seq_no, void* ptr) {
    DATA_ELEMENT* el = (DATA_ELEMENT*)ptr;
    uint64_t begin = get_milliseconds();
    cout << "[train] latency trigger: " << begin - el->publish_ts << endl;

    // check if the latest model is still fresh
    unsigned long latest_model_index = dht_latest_index((char*)TOPIC_REGRESSOR_MODEL);
    if (WooFInvalid(latest_model_index)) {
        cerr << "[train] failed to get the latest index from " << TOPIC_REGRESSOR_MODEL << ": " << dht_error_msg
             << endl;
        exit(1);
    }
    RAFT_DATA_TYPE data = {0};
    REGRESSOR_MODEL latest_model = {0};
    if (latest_model_index > 0) {
        if (dht_get((char*)TOPIC_REGRESSOR_MODEL, &data, latest_model_index) < 0) {
            cerr << "[train] failed to get data from " << TOPIC_REGRESSOR_MODEL << "[" << latest_model_index
                 << "]: " << dht_error_msg << endl;
            exit(1);
        }
        memcpy(&latest_model, &data, sizeof(REGRESSOR_MODEL));
        if (latest_model.ts >= el->ts - MODEL_EXPIRATION) {
            // cout << "[train] latest model was trained " << (el->ts - latest_model.ts) / 1000 << " seconds ago" << endl;
            // cout << "[train] " << (MODEL_EXPIRATION - (el->ts - latest_model.ts)) / 1000
            //      << " seconds until next training" << endl;
            return 1;
        }
    }

    double dht_reading[MAX_SAMPLES] = {0};
    double cpu_reading[TOPIC_NUMBER][MAX_SAMPLES] = {0};
    unsigned long dht_index = dht_latest_earlier_index(topic_name, seq_no);
    if (WooFInvalid(dht_index)) {
        cerr << "[train] failed to get the latest index from " << topic_name << ": " << dht_error_msg << endl;
        exit(1);
    }
    cout << "[train] " << topic_name << " has " << dht_index << " data points" << endl;
    if (latest_model_index == 0 && dht_index != MIN_SAMPLES) {
        cout << "[train] " << MIN_SAMPLES - dht_index << " more to start training" << endl;
        return 1;
    }
    cout << "[train] start collecting data for training" << endl;
    unsigned long cpu_index[TOPIC_NUMBER] = {0};
    for (int i = 0; i < TOPIC_NUMBER; ++i) {
        char cpu_topic[DHT_NAME_LENGTH] = {0};
        cpu_topic_name(i, cpu_topic);
        cpu_index[i] = dht_latest_index(cpu_topic);
        if (WooFInvalid(cpu_index[i])) {
            cerr << "[train] failed to get the latest index from " << cpu_topic << ": " << dht_error_msg << endl;
            exit(1);
        }
        cout << "[train] " << cpu_topic << " has " << cpu_index[i] << " data points" << endl;
    }
    int cnt = 0;
    while (cnt < MAX_SAMPLES && dht_index > 0) {
        if (dht_get(topic_name, &data, dht_index) < 0) {
            cerr << "[train] failed to get data from " << topic_name << "[" << dht_index << "]: " << dht_error_msg
                 << endl;
            exit(1);
        }
        DATA_ELEMENT* dht_data = (DATA_ELEMENT*)&data;
        dht_reading[cnt] = dht_data->val;
        uint64_t dht_ts = dht_data->ts;
        // cout << "[train] dht_ts: " << dht_ts << " (" << dht_index << ")" << endl;
        if (dht_ts <= el->ts - TRAINING_WINDOW) {
            cout << "[train] passed training window, latest_dht_ts: " << el->ts << endl;
            break;
        }
        bool missing_data = false;
        for (int i = 0; i < TOPIC_NUMBER && !missing_data; ++i) {
            char cpu_topic[DHT_NAME_LENGTH] = {0};
            cpu_topic_name(i, cpu_topic);
            while (cpu_index[i] > 0) {
                if (dht_get(cpu_topic, &data, cpu_index[i]) < 0) {
                    cerr << "[train] failed to get data from " << cpu_topic << "[" << cpu_index[i]
                         << "]: " << dht_error_msg << endl;
                    exit(1);
                }
                DATA_ELEMENT* cpu_data = (DATA_ELEMENT*)&data;
                // cout << "[train] cpu[" << i + 1 << "]_ts: " << cpu_data->ts << "(" << cpu_index[i] << ")" << endl;
                if (cpu_data->ts == dht_ts) {
                    cpu_reading[i][cnt] = cpu_data->val;
                    --cpu_index[i];
                    break;
                } else if (cpu_data->ts < dht_ts) {
                    missing_data = true;
                    break;
                }
                --cpu_index[i];
            }
        }
        if (!missing_data) {
            ++cnt;
            // cout << "[train] collected data for " << dht_ts << endl;
        } else {
            // cout << "[train] missing data for " << dht_ts << endl;
        }
        --dht_index;
    }

    if (cnt == 0) {
        cout << "[train] no sample collected" << endl;
        return 1;
    } else if (cnt < MIN_SAMPLES && dht_index > 0) {
        cout << "[train] collected " << cnt << " samples" << endl;
        cout << "[train] doesn't meet minimum samle requirement, use the last model instead" << endl;
        latest_model.ts = el->ts;
        if (dht_publish((char*)TOPIC_REGRESSOR_MODEL, &latest_model, sizeof(REGRESSOR_MODEL)) < 0) {
            cerr << "[train] failed to publish trained model: " << dht_error_msg << endl;
            exit(1);
        }
        return 1;
    }
    cout << "[train] start training with " << cnt << " samples" << endl;
    arma::mat pred(TOPIC_NUMBER, cnt);
    arma::rowvec resp(cnt);
    for (int i = 0; i < cnt; ++i) {
        for (int j = 0; j < TOPIC_NUMBER; ++j) {
            pred(j, i) = cpu_reading[j][i];
            resp(i) = dht_reading[i];
        }
    }

    mlpack::regression::LinearRegression regressor;
    regressor.Train(pred, resp);
    arma::vec& p = regressor.Parameters();
    // cout << "[train] trained regressor parameters: " << p(0) << " " << p(1) << " " << p(2) << " " << p(3) << " " <<
    // p(4)
    //      << endl;

    cout << "[train] training took " << (get_milliseconds() - begin) / 1000 << " seconds" << endl;
    REGRESSOR_MODEL model = {0};
    store_model(&model, regressor);
    model.ts = el->ts;
    if (dht_publish((char*)TOPIC_REGRESSOR_MODEL, &model, sizeof(REGRESSOR_MODEL)) < 0) {
        cerr << "[train] failed to publish trained model: " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
