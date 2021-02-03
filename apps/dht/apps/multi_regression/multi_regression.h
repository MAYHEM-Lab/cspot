#ifndef MULTI_REGRESSION_H
#define MULTI_REGRESSION_H

#include "dht.h"
#include "stdint.h"

#define TOPIC_CPU_1 "cpu_1"
#define TOPIC_CPU_2 "cpu_2"
#define TOPIC_CPU_3 "cpu_3"
#define TOPIC_CPU_4 "cpu_4"
#define TOPIC_DHT_1 "dht_1"
#define TOPIC_SMOOTH_SUFFIX "_smooth"
#define TOPIC_PREDICT "predict"
#define TOPIC_ERROR "error"
#define TOPIC_REGRESSOR_MODEL "model"
#define HANDLER_PREDICT "h_predict"
#define HANDLER_SMOOTH "h_smooth"
#define HANDLER_TRAIN "h_train"
#define TOPIC_NUMBER 4
#define MULTI_REGRESSION_HISTORY_LENGTH 1024

#define align_ts(ts) (ts - (ts % 300000) + (ts % 300000 >= 150000 ? 300000 : 0))

typedef struct data_element {
    double val;
    uint64_t ts;
} DATA_ELEMENT;

typedef struct regressor_model {
    int32_t n_rows;
    int32_t n_cols;
    int32_t n_elem;
    double p_vec[8];
    double lambda;
    uint64_t ts;
} REGRESSOR_MODEL;

#endif