#ifndef MULTI_REGRESSION_H
#define MULTI_REGRESSION_H

#include "dht.h"

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

#define MULTI_REGRESSION_HISTORY_LENGTH 256
#define TRAINING_WINDOW 5

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
    unsigned long timestamp;
} TEMPERATURE_ELEMENT;

typedef struct regressor_model {
    int n_rows;
    int n_cols;
    int n_elem;
    double p_vec[8];
    double lambda;
} REGRESSOR_MODEL;

// typedef struct regressing_args {
//     int window;
// } REGRESSING_ARGS;

#endif