#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_HANDLER "test_dht_handler"
#define ARGS "t:r:l:p:"
char* Usage = "test_trigger_publish_multi -t topic -r rate -l duration(second) -p thread_count\n";

typedef struct test_stc {
    char msg[256 - 8];
    uint64_t sent;
} TEST_EL;

typedef struct test_arg {
    uint64_t begin;
    uint64_t duration;
    int done;
    int rate;
    pthread_mutex_t lock;
    char topic_name[DHT_NAME_LENGTH];
} TEST_ARG;

void* publish(void* ptr) {
    TEST_ARG* arg = (TEST_ARG*)ptr;
    uint64_t now = get_milliseconds();
    while (now - arg->begin <= arg->duration) {
        pthread_mutex_lock(&arg->lock);
        int expected = (now - arg->begin) * arg->rate / 1000;
        if (arg->done < expected) {
            ++arg->done;
            TEST_EL el = {0};
            sprintf(el.msg, "test_%d", arg->done);
            pthread_mutex_unlock(&arg->lock);
            el.sent = get_milliseconds();

            int err = dht_publish(arg->topic_name, &el, sizeof(TEST_EL));
            if (err < 0) {
                fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
                exit(1);
            }
            printf("%s published at %lu\n", el.msg, get_milliseconds());
            int remaining = (arg->duration - (now - arg->begin)) / 1000;
            int hour = remaining / 60 / 60;
            int minute = (remaining / 60) % 60;
            int second = remaining % 60;
            printf("%02d:%02d:%02d remaining\n", hour, minute, second);
            fflush(stdout);
        } else {
            pthread_mutex_unlock(&arg->lock);
            usleep(1000);
        }
        now = get_milliseconds();
    }
}

int main(int argc, char** argv) {
    int thread_count = 16;
    TEST_ARG test_arg = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(test_arg.topic_name, optarg, sizeof(test_arg.topic_name));
            break;
        }
        case 'r': {
            test_arg.rate = (int)strtoul(optarg, NULL, 10);
            break;
        }
        case 'l': {
            test_arg.duration = (int)strtoul(optarg, NULL, 10) * 1000;
            break;
        }
        case 'p': {
            thread_count = (int)strtoul(optarg, NULL, 10);
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (test_arg.topic_name[0] == 0 || test_arg.rate == 0 || test_arg.duration == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    test_arg.begin = get_milliseconds();
    pthread_mutex_init(&test_arg.lock, NULL);
    WooFInit();

    pthread_t* pid = malloc(thread_count * sizeof(pthread_t));
    int i;
    for (i = 0; i < thread_count; ++i) {
        if (pthread_create(&pid[i], NULL, publish, &test_arg) < 0) {
            fprintf(stderr, "failed to creat pthread\n");
            free(pid);
            exit(1);
        }
    }
    printf("created %d threads to publish\n", thread_count);
    for (i = 0; i < thread_count; ++i) {
        if (pthread_join(pid[i], NULL) < 0) {
            fprintf(stderr, "failed to join pthread\n");
            free(pid);
            exit(1);
        }
    }
    free(pid);
    printf("total: %d\n", test_arg.done);

    return 0;
}
