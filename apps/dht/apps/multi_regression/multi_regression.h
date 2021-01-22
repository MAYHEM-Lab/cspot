#ifndef MULTI_REGRESSION_H
#define MULTI_REGRESSION_H

#include "dht.h"
#include "stdint.h"

#define TOPIC_PIZERO02_CPU "pizero02_cpu"
#define TOPIC_PIZERO02_DHT "pizero02_dht"
#define TOPIC_PIZERO04_CPU "pizero04_cpu"
#define TOPIC_PIZERO04_DHT "pizero04_dht"
#define TOPIC_PIZERO05_CPU "pizero05_cpu"
#define TOPIC_PIZERO05_DHT "pizero05_dht"
#define TOPIC_PIZERO06_CPU "pizero06_cpu"
#define TOPIC_PIZERO06_DHT "pizero06_dht"
#define TOPIC_WU_STATION "wu_station"
#define TOPIC_PIZERO02_PREDICT "pizero02_predict"
#define TOPIC_REGRESSOR_MODEL "regressor_model"
#define HANDLER_REGRESS "h_regress"
#define HANDLER_TRAIN "h_train"
#define CLIENT_WOOF_NAME "client_publish.woof"
#define CLIENT_WOOF_LENGTH 16

#define MULTI_REGRESSION_HISTORY_LENGTH 256

char reading_topics[][DHT_NAME_LENGTH] = {TOPIC_PIZERO02_CPU,
                                          TOPIC_PIZERO02_DHT,
                                          TOPIC_PIZERO04_CPU,
                                          TOPIC_PIZERO04_DHT,
                                          TOPIC_PIZERO05_CPU,
                                          TOPIC_PIZERO05_DHT,
                                          TOPIC_PIZERO06_CPU,
                                          TOPIC_PIZERO06_DHT,
                                          TOPIC_WU_STATION};
char other_topics[][DHT_NAME_LENGTH] = {TOPIC_PIZERO02_PREDICT, TOPIC_REGRESSOR_MODEL};

typedef struct temperature_element {
    double temp;
    uint64_t timestamp;
} TEMPERATURE_ELEMENT;

typedef struct publish_arg {
    char topic[DHT_NAME_LENGTH];
    TEMPERATURE_ELEMENT val;
} PUBLISH_ARG;

typedef struct regressor_model {
    int32_t n_rows;
    int32_t n_cols;
    int32_t n_elem;
    double p_vec[8];
    double lambda;
} REGRESSOR_MODEL;

#endif