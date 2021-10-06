#include "dht_client.h"
#include "dht_utils.h"
#include "multi_regression.h"

#include <algorithm>
#include <cstdint>
#include <iostream>

#define SMOOTH_WINDOW (60 * 60 * 1000)

using namespace std;

extern "C" int h_smooth(char* topic_name, unsigned long seq_no, void* ptr) {
    DATA_ELEMENT* el = (DATA_ELEMENT*)ptr;

    uint64_t triggered = get_milliseconds();
    cout << "[smooth] latency trigger: " << triggered - el->publish_ts << endl;
    double sum = el->val;
    int cnt = 1;
    uint64_t latest_ts = align_ts(el->ts);
    unsigned long latest_index = dht_latest_earlier_index(topic_name, seq_no);
    if (WooFInvalid(latest_index)) {
        cerr << "[smooth] failed to get the latest index of " << topic_name << ": " << dht_error_msg << endl;
    } else if (latest_index == 0) {
        cerr << "[smooth] no data in " << topic_name << ", which is impossible" << endl;
    } else {
        for (unsigned long i = latest_index - 1; i != 0; --i) {
            RAFT_DATA_TYPE data = {0};
            if (dht_get(topic_name, &data, i) < 0) {
                cerr << "[smooth] failed to get data from " << topic_name << "[" << i << "]: " << dht_error_msg << endl;
            }
            DATA_ELEMENT* reading = (DATA_ELEMENT*)&data;
            uint64_t reading_ts = align_ts(reading->ts);
            if (reading_ts <= latest_ts - SMOOTH_WINDOW) {
                break;
            }
            sum += reading->val;
            ++cnt;
        }
    }
    DATA_ELEMENT avg = {0};
    avg.val = sum / cnt;
    avg.ts = latest_ts;
    avg.publish_ts = get_milliseconds();
    // cout << "[smooth] " << avg.ts << " average " << cnt << " readings in " << topic_name << ": " << avg.val << endl;
    cout << "[smooth] latency smooth: " << get_milliseconds() - triggered << endl;
    char smooth_topic[DHT_NAME_LENGTH] = {0};
    sprintf(smooth_topic, "%s%s", topic_name, TOPIC_SMOOTH_SUFFIX);
    if (dht_publish(smooth_topic, &avg, sizeof(DATA_ELEMENT)) < 0) {
        cerr << "[smooth] failed to publish to " << smooth_topic << ": " << dht_error_msg << endl;
        exit(1);
    }

    return 1;
}
