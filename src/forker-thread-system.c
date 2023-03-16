#include "log.h"
#include "woofc-access.h"
#include "woofc.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char WooF_dir[2048];
char WooF_namespace[2048];
char WooF_namelog_dir[2048];
char Host_ip[25];
char Namelog_name[2048];
unsigned long Name_id;
LOG* Name_log;

static int WooFDone;

void* WooFForker(void* arg);

/*
 * FIX ME: it would be better if the TRIGGER seq_no gets retired after the launch is
 * successful Right now, the last_seq_no in the launcher is set before the launch actually
 * happens which means a failure will nt be retried
 */
void* WooFHandlerThread(void* arg) {
    char* launch_string = (char*)arg;

#ifdef DEBUG
    fprintf(stdout, "LAUNCH: %s\n", launch_string);
    fflush(stdout);
#endif
    system(launch_string);
#ifdef DEBUG
    fprintf(stdout, "DONE: %s\n", launch_string);
    fflush(stdout);
#endif

    free(launch_string);

    pthread_exit(NULL);
    return (NULL);
}

void* WooFForker(void* arg) {
    unsigned long last_seq_no = 0;
    unsigned long first;
    unsigned long last;
    LOG* log_tail;
    EVENT* ev;
    char* launch_string;
    char* pathp;
    WOOF* wf;
    char woof_shepherd_dir[2048];
    int err;
    pthread_t tid;
    int none;
    EVENT last_event; /* needed to understand if log tail has changed */

    /*
     * wait for things to show up in the log
     */
    memset(&last_event, 0, sizeof(last_event));
#ifdef DEBUG
    fprintf(stdout, "WooFForker: namespace: %s started\n", WooF_namespace);
    fflush(stdout);
#endif

#ifdef DEBUG
    fprintf(stdout, "WooFForker: namespace: %s memset called\n", WooF_namespace);
    fflush(stdout);
#endif

    while (WooFDone == 0) {
#ifdef DEBUG
        fprintf(stdout, "WooFForker: namespace: %s caling P\n", WooF_namespace);
        fflush(stdout);
#endif
        P(&Name_log->tail_wait);
#ifdef DEBUG
        fprintf(stdout, "WooFForker: namespace: %s awake\n", WooF_namespace);
        fflush(stdout);
#endif

        pthread_yield();

        if (WooFDone == 1) {
            break;
        }

        /*
         * must lock to extract tail
         */
        P(&Name_log->mutex);
#ifdef DEBUG
        fprintf(stdout, "WooFForker: namespace: %s, in mutex, size: %lu\n", WooF_namespace, Name_log->size);
        fflush(stdout);
#endif
        log_tail = LogTail(Name_log, last_seq_no, Name_log->size);
        V(&Name_log->mutex);
#ifdef DEBUG
        fprintf(stdout, "WooFForker: namespace: %s out of mutex with log tail\n", WooF_namespace);
        fflush(stdout);
#endif

        if (log_tail == NULL) {
#ifdef DEBUG
            fprintf(stdout, "WooFForker: namespace: %s no tail, continuing\n", WooF_namespace);
            fflush(stdout);
#endif
            continue;
        }
        if (log_tail->head == log_tail->tail) {
#ifdef DEBUG
            fprintf(stdout, "WooFForker: namespace: %s log tail empty, continuing\n", WooF_namespace);
            fflush(stdout);
#endif
            LogFree(log_tail);
            continue;
        }

        ev = (EVENT*)(MIOAddr(log_tail->m_buf) + sizeof(LOG));
#ifdef DEBUG
        first = log_tail->head;
        printf("WooFForker: namespace: %s first: %lu\n", WooF_namespace, first);
        fflush(stdout);
        while (first != log_tail->tail) {
            printf("WooFForker: namespace: %s first %lu, seq_no: %lu, last %lu evns: %s\n",
                   WooF_namespace,
                   first,
                   ev[first].seq_no,
                   last_seq_no,
                   ev[first].namespace);
            fflush(stdout);
            if ((ev[first].type == TRIGGER) &&
                (strncmp(ev[first].namespace, WooF_namespace, sizeof(ev[first].namespace)) == 0)) {
                break;
            }
            first = (first - 1);
//BUG            if (first >= log_tail->size) {
           if (first < 0) {
                first = log_tail->size - 1;
            }
        }
#endif

        /*
         * find the first TRIGGER we haven't seen yet
         * skip triggers for other name spaces but try and wake them up
         */
        none = 0;
        first = log_tail->head;

        while (first != log_tail->tail) {
            if ((ev[first].type == TRIGGER) &&
                (strncmp(ev[first].namespace, WooF_namespace, sizeof(ev[first].namespace)) == 0) &&
                (ev[first].seq_no > last_seq_no)) {
                /* found one */
                break;
            }

            /*
             * if this is a trigger belonging to another namespace, try and wake
             * its container(s)
             *
             * Note that this may cause spurious wake ups since this container
             * can't tell if the others have seen this trigger yet
             */
            if ((ev[first].type == TRIGGER) && (memcmp(&last_event, &ev[first], sizeof(last_event)) != 0) &&
                (strncmp(ev[first].namespace, WooF_namespace, sizeof(ev[first].namespace)) != 0)) {
#ifdef DEBUG
                printf("WooFForker: namespace: %s found trigger for evns: %s, name: %s, "
                       "first: %lu, head: %lu, tail: %lu\n",
                       WooF_namespace,
                       ev[first].namespace,
                       ev[first].woofc_name,
                       first,
                       log_tail->head,
                       log_tail->tail);
                fflush(stdout);
#endif
                exit(1);
                memcpy(&last_event, &ev[first], sizeof(last_event));
                V(&Name_log->tail_wait);
                pthread_yield();
            }

            first = (first - 1);
//BUG            if (first >= log_tail->size) {
            if (first < 0) {
                first = log_tail->size - 1;
            }
            if (first == log_tail->tail) {
                none = 1;
                break;
            }
        }

        /*
         * if no TRIGGERS found
         */
        if (none == 1) {
#ifdef DEBUG
            fprintf(stdout, "WooFForker log tail empty, continuing\n");
            fflush(stdout);
#endif
            LogFree(log_tail);
            continue;
        }
        /*
         * otherwise, fire this event
         */

#ifdef DEBUG
        fprintf(stdout,
                "WooFForker: namespace: %s firing woof: %s handler: %s woof_seq_no: %lu "
                "log_seq_no: %lu\n",
                WooF_namespace,
                ev[first].woofc_name,
                ev[first].woofc_handler,
                ev[first].woofc_seq_no,
                ev[first].seq_no);
        fflush(stdout);
#endif

        wf = WooFOpen(ev[first].woofc_name);

        if (wf == NULL) {
            fprintf(stderr,
                    "WooFForker: open failed for WooF at %s, %lu %lu\n",
                    ev[first].woofc_name,
                    ev[first].woofc_element_size,
                    ev[first].woofc_history_size);
            fflush(stderr);
            exit(1);
        }

        /*
         * find the last directory in the path
         */
        pathp = strrchr(WooF_dir, '/');
        if (pathp == NULL) {
            fprintf(stderr, "couldn't find leaf dir in %s\n", WooF_dir);
            exit(1);
        }

        strncpy(woof_shepherd_dir, pathp, sizeof(woof_shepherd_dir));

        launch_string = (char*)malloc(2048);
        if (launch_string == NULL) {
            exit(1);
        }

        memset(launch_string, 0, 2048);

        sprintf(launch_string,
                "export WOOFC_NAMESPACE=%s; \
			 export WOOFC_DIR=%s; \
			 export WOOC_HOST_IP=%s; \
			 export WOOF_SHEPHERD_NAME=%s; \
			 export WOOF_SHEPHERD_NDX=%lu; \
			 export WOOF_SHEPHERD_SEQNO=%lu; \
			 export WOOF_NAME_ID=%lu; \
			 export WOOF_NAMELOG_DIR=%s; \
			 export WOOF_NAMELOG_NAME=%s; \
			 export WOOF_NAMELOG_SEQNO=%lu; \
			 %s/%s",
                WooF_namespace,
                WooF_dir,
                Host_ip,
                wf->shared->filename,
                ev[first].woofc_ndx,
                ev[first].woofc_seq_no,
                Name_id,
                WooF_namelog_dir,
                Namelog_name,
                ev[first].seq_no,
                WooF_dir,
                ev[first].woofc_handler);

        /*
         * remember its sequence number for next time
         *
         * needs +1 because LogTail returns the earliest inclusively
         */
        last_seq_no = ev[first].seq_no; /* log seq_no */
#ifdef DEBUG
        fprintf(stdout,
                "WooFForker: namespace: %s seq_no: %lu, handler: %s\n",
                WooF_namespace,
                ev[first].seq_no,
                ev[first].woofc_handler);
        fflush(stdout);
#endif
        LogFree(log_tail);
        WooFDrop(wf);

        err = pthread_create(&tid, NULL, WooFHandlerThread, (void*)launch_string);
        if (err < 0) {
            /* LOGGING
             * log thread create failure here
             */
            exit(1);
        }
        pthread_detach(tid);
    }

#ifdef DEBUG
    fprintf(stdout, "WooFForker namespace: %s exiting\n", WooF_namespace);
    fflush(stdout);
#endif

    pthread_exit(NULL);
}
