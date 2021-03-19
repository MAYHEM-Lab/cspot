#include "MQTTAsync.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ADDRESS "tcp://localhost:1883"
#define TOPIC "mqtt/test/topic"
#define SIZE 8192
#define TIMEOUT 10000L

volatile int finished = 0;
volatile int connected = 0;
volatile int sent = 0;
volatile int failed = 0;

void connlost(void* context, char* cause) {
    printf("\nConnection lost\n");
    finished = 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Disconnect failed\n");
    finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) {
    printf("Successful disconnection\n");
    finished = 1;
}

void onSendFailure(void* context, MQTTAsync_failureData* response) {
    printf("Message send failed token %d error code %d\n", response->token, response->code);
    fflush(stdout);
    ++failed;
}

void onSend(void* context, MQTTAsync_successData* response) {
    printf("Message token %d sent\n", response->token);
    fflush(stdout);
    ++sent;
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
        printf("Connect failed, rc %d\n", response ? response->code : 0);
    finished = 1;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("Successful connection\n");
    connected = 1;
}

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m) {
    return 1;
}

int64_t getTime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((int64_t)ts.tv_sec * 1000) + ((int64_t)ts.tv_nsec / 1000000);
}

int main(int argc, char* argv[]) {
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
    int rc, qos, rate, duration;

    if (argc < 4) {
        printf("./mqttc_pub QoS rate(/sec) duration(sec)\n");
        exit(1);
    }
    qos = atoi(argv[1]);
    rate = atoi(argv[2]);
    duration = atoi(argv[3]);

    char client_id[256] = {0};
    srand(time(NULL));
    sprintf(client_id, "mqtt_%d", rand());
    rc = MQTTAsync_create(&client, ADDRESS, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to create client object, return code %d\n", rc);
        exit(1);
    }

    rc = MQTTAsync_setCallbacks(client, NULL, connlost, messageArrived, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to set callback, return code %d\n", rc);
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
        exit(1);
    }

    while (!connected) {
        usleep(100000L);
    }

    int64_t begin = getTime();
    int done = 0;
    while (!finished) {
        int64_t ts = getTime();
        if (ts - begin > duration * 1000) {
            break;
        }
        if (done < (ts - begin) * rate / 1000) {
            char buf[SIZE] = {0};
            int n = snprintf(buf, sizeof(buf), "%lld", (long long)ts);
            printf("%s\n", buf);
            fflush(stdout);

            pub_opts.onSuccess = onSend;
            pub_opts.onFailure = onSendFailure;
            pub_opts.context = client;

            pubmsg.payload = buf;
            pubmsg.payloadlen = SIZE;
            pubmsg.qos = qos;
            pubmsg.retained = 1;

            rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &pub_opts);
            if (rc != MQTTASYNC_SUCCESS) {
                printf("Failed to start sendMessage, return code %d\n", rc);
                exit(1);
            }
            ++done;
        } else {
            usleep(1000);
        }
    }

    while (sent + failed < done) {
        usleep(10000);
    }
    MQTTAsync_destroy(&client);
    return 0;
}
