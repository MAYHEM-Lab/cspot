#include "MQTTAsync.h"
#include "mqttc_test.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "mqtt_paho_sub"
#define TOPIC "mqtt/test/topic"
#define TIMEOUT 10000L

int disc_finished = 0;
int subscribed = 0;
int finished = 0;
int qos = 0;
volatile int received = 0;
volatile long long diff = 0;

uint64_t get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((uint64_t)ts.tv_sec * 1000) + ((uint64_t)ts.tv_nsec / 1000000);
}

void connlost(void* context, char* cause) {
    printf("\nConnection lost\n");
    finished = 1;
}

int msgarrvd(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    char buf[8192];
    memcpy(buf, message->payload, 8192);
    char woofname[256] = {0};
    sprintf(woofname, "woof://128.111.45.112/home/centos/cspot_val1/apps/mqttc_test/cspot/%s", MQTTC_WOOFNAME);
    unsigned long seq = WooFPut(woofname, MQTTC_HANDLER, buf);
    if (WooFInvalid(seq)) {
        printf("failed to WooFPut to %s\n", woofname);
        fflush(stdout);
        return 1;
    }
    sprintf(woofname, "woof://128.111.45.132/home/centos/cspot_val2/apps/mqttc_test/cspot/%s", MQTTC_WOOFNAME);
    seq = WooFPut(woofname, MQTTC_HANDLER, buf);
    if (WooFInvalid(seq)) {
        printf("failed to WooFPut to %s\n", woofname);
        fflush(stdout);
        return 1;
    }
    sprintf(woofname, "woof://128.111.45.133/home/centos/cspot_val3/apps/mqttc_test/cspot/%s", MQTTC_WOOFNAME);
    seq = WooFPut(woofname, MQTTC_HANDLER, buf);
    if (WooFInvalid(seq)) {
        printf("failed to WooFPut to %s\n", woofname);
        fflush(stdout);
        return 1;
    }

    int64_t ts = strtoul(message->payload, NULL, 0);
    ++received;
    diff += (get_time() - ts);
    if (received % 100 == 0) {
        printf("%d: %lld\n", received, diff / received);
        fflush(stdout);
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Disconnect failed, rc %d\n", response->code);
    disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) {
    printf("Successful disconnection\n");
    disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response) {
    printf("Subscribe succeeded\n");
    subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    printf("Subscribe failed, rc %d\n", response->code);
    finished = 1;
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Connect failed, rc %d\n", response->code);
    finished = 1;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;

    printf("Successful connection\n");
    printf("Subscribing to topic %s, QoS %d\n", TOPIC, qos);
    printf("Press Q<Enter> to quit\n");
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = client;
    rc = MQTTAsync_subscribe(client, TOPIC, qos, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start subscribe, return code %d\n", rc);
        finished = 1;
    }
}

int main(int argc, char* argv[]) {
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    int rc, ch;
    if (argc < 2) {
        printf("./mqttc_sub QoS\n");
        exit(1);
    }
    qos = atoi(argv[1]);

    WooFInit();
    WooFMsgCacheInit();

    rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to create client, return code %d\n", rc);
        WooFMsgCacheShutdown();
        exit(1);
    }
    rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to set callbacks, return code %d\n", rc);
        MQTTAsync_destroy(&client);
        WooFMsgCacheShutdown();
        exit(1);
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;
    // conn_opts.maxInflight = 1;
    rc = MQTTAsync_connect(client, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        MQTTAsync_destroy(&client);
        WooFMsgCacheShutdown();
        exit(1);
    }

    while (!subscribed && !finished) {
        usleep(10000L);
    }

    if (finished) {
        WooFMsgCacheShutdown();
        exit(1);
    }

    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');
    printf("diff: %lld\n", diff);
    printf("received: %d\n", received);
    printf("avg: %lld\n", diff / received);

    disc_opts.onSuccess = onDisconnect;
    disc_opts.onFailure = onDisconnectFailure;
    rc = MQTTAsync_disconnect(client, &disc_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start disconnect, return code %d\n", rc);
        MQTTAsync_destroy(&client);
        WooFMsgCacheShutdown();
        exit(1);
    }
    while (!disc_finished) {
        usleep(10000L);
    }

    MQTTAsync_destroy(&client);
    WooFMsgCacheShutdown();
    return rc;
}