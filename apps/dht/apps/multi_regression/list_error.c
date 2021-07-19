#include "dht_client.h"
#include "multi_regression.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    WooFInit();

    double min_error = 99, max_error = 0, sum = 0;
    int cnt = 0;
    unsigned long last_index = dht_latest_index(TOPIC_ERROR);
    unsigned long i;
    for (i = 1; i <= last_index; ++i) {
        RAFT_DATA_TYPE data = {0};
        if (dht_get(TOPIC_ERROR, &data, i) < 0) {
            fprintf(stderr, "failed to get error at %lu\n", i);
            exit(1);
        }
        double error;
        memcpy(&error, data.val, sizeof(double));
        if (error < min_error) {
            min_error = error;
        }
        if (error > max_error) {
            max_error = error;
        }
        sum += error;
        ++cnt;
        printf("%f ", error);
    }
    printf("\n");
    printf("min: %f, max: %f, avg: %f\n", min_error, max_error, sum / cnt);

    return 0;
}
