extern "C" {
#include "log.h"
#include "woofc.h"
}

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <woofc-access.h>
#include <iostream>

char WooF_dir[2048];
char WooF_namespace[2048];
char WooF_namelog_dir[2048];
char Host_dir[2048];
char Host_ip[25];
char Namelog_name[2048];
unsigned long Name_id;
LOG* Name_log;
char** WooF_worker_containers = NULL;
static int WooFDone;

struct cont_arg_stc {
    int container_count;
};

typedef struct cont_arg_stc CA;

// #define DEBUG
void WooFShutdown(int sig) {
    int val;

    WooFDone = 1;
    while (sem_getvalue(&Name_log->tail_wait, &val) >= 0) {
        if (val > 0) {
            break;
        }
        V(&Name_log->tail_wait);
    }

    return;
}

int WooFInit() {
    struct timeval tm;
    int err;
    char log_name[2048];
    char log_path[2048];
    char putbuf[1024];
    char* str;
    MIO* lmio;
    unsigned long name_id;

    gettimeofday(&tm, NULL);
    srand48(tm.tv_sec + tm.tv_usec);

    str = getenv("WOOFC_DIR");
    if (str == NULL) {
        getcwd(WooF_dir, sizeof(WooF_dir));
    } else {
        strncpy(WooF_dir, str, sizeof(WooF_dir));
    }

    if (strcmp(WooF_dir, "/") == 0) {
        fprintf(stderr, "WooFInit: WOOFC_DIR can't be %s\n", WooF_dir);
        exit(1);
    }

    if (WooF_dir[strlen(WooF_dir) - 1] == '/') {
        WooF_dir[strlen(WooF_dir) - 1] = 0;
    }

    memset(putbuf, 0, sizeof(putbuf));
    sprintf(putbuf, "WOOFC_DIR=%s", WooF_dir);
    putenv(putbuf);
#ifdef DEBUG
    fprintf(stdout, "WooFInit: WooF_dir: %s\n", putbuf);
    fflush(stdout);
#endif

    /*
     * note that the container will always have WOOFC_NAMESPACE specified by the launcher
     */
    str = getenv("WOOFC_NAMESPACE");
    if (str == NULL) { /* assume this is the host */
        /* use the WooF_dir as the namespace */
        strncpy(WooF_namespace, WooF_dir, sizeof(WooF_namespace));
    } else { /* in the container */
        strncpy(WooF_namespace, str, sizeof(WooF_namespace));
    }
#ifdef DEBUG
    fprintf(stdout, "WooFInit: namespace: %s\n", WooF_namespace);
    fflush(stdout);
#endif

    str = getenv("WOOF_NAMELOG_DIR");
    if (str == NULL) { /* if not specified */
        /* use the WooF_dir as the namelog dir */
        strncpy(WooF_namelog_dir, WooF_dir, sizeof(WooF_namespace));
    } else { /* in the container */
        strncpy(WooF_namelog_dir, str, sizeof(WooF_namelog_dir));
    }

#ifdef DEBUG
    fprintf(stdout, "WooFInit: namelog dir: %s\n", WooF_namelog_dir);
    fflush(stdout);
#endif

    err = mkdir(WooF_dir, 0600);
    if ((err < 0) && (errno != EEXIST)) {
        perror("WooFInit");
        return (-1);
    }

    name_id = WooFNameHash(WooF_namelog_dir);
    sprintf(Namelog_name, "cspot-log.%20.20lu", name_id);
    memset(log_name, 0, sizeof(log_name));
    sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);

#ifdef DEBUG
    printf("WooFInit: Name log at %s with name %s\n", log_name, Namelog_name);
    fflush(stdout);
#endif

#ifndef IS_PLATFORM
    lmio = MIOReOpen(log_name);
    if (lmio == NULL) {
        fprintf(stderr, "WooFInit: name %s (%s) log not initialized.\n", log_name, WooF_dir);
        fflush(stderr);
        exit(1);
    }
    Name_log = MIOAddr(lmio);

#endif
    Name_id = name_id;

    return (1);
}

#ifdef IS_PLATFORM
void* WooFLauncher(void* arg);
void* WooFContainerLauncher(void* arg);

void CatchSignals();

void CleanUpDocker(int, void*);
void CleanUpContainers(char** names);

static int WooFHostInit(int min_containers, int max_containers) {
    char log_name[2048];
    CA* ca;
    int err;
    pthread_t tid;

    WooFInit();

    memset(log_name, 0, sizeof(log_name));
    sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);

    Name_log = LogCreate(log_name, Name_id, DEFAULT_WOOF_LOG_SIZE);
    if (Name_log == NULL) {
        fprintf(stderr, "WooFInit: couldn't create name log as %s, size %d\n", log_name, DEFAULT_WOOF_LOG_SIZE);
        fflush(stderr);
        exit(1);
    }
#ifdef DEBUG
    printf("WooFHostInit: created %s\n", log_name);
    fflush(stdout);
#endif

    ca = new CA;
    if (ca == NULL) {
        exit(1);
    }

    ca->container_count = min_containers;

    err = pthread_create(&tid, NULL, WooFContainerLauncher, (void*)ca);
    if (err < 0) {
        fprintf(stderr, "couldn't start launcher thread\n");
        exit(1);
    }

    /*
     * sleep for 10 years
     *
     * calling pthread_join here causes pthreads to intercept the SIGINT signal handler
     * (which sounds like a bug in Linux).  Sleeping forever, however, allows SIGINT to
     * be caught which then triggers a clean up of the docker container
     */
    sleep(86400 * 365 * 10);
    pthread_join(tid, NULL);

    exit(0);
    return (1);
}

void WooFExit() {
    WooFDone = 1;
    pthread_exit(NULL);
}

/*
 * FIX ME: it would be better if the TRIGGER seq_no gets
 * retired after the launch is successful  Right now, the last_seq_no
 * in the launcher is set before the launch actually happens
 * which means a failure will not be retried
 */
void* WooFDockerThread(void* arg) {
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

    return (NULL);
}

void* WooFContainerLauncher(void* arg) {
    unsigned long last_seq_no = 0;
    unsigned long first;
    LOG* log_tail;
    EVENT* ev;
    char* launch_string;
    char* pathp;
    WOOF* wf;
    int err;
    pthread_t tid;
    int none;
    int count;
    int container_count;
    unsigned int port;
    CA* ca = (CA*)arg;
    pthread_t* tids;

    container_count = ca->container_count;
    free(ca);

    /*
     * find the last directory in the path
     */
    pathp = strrchr(WooF_dir, '/');
    if (pathp == NULL) {
        fprintf(stderr, "couldn't find leaf dir in %s\n", WooF_dir);
        exit(1);
    }

    // build the names for the workers to spawn
    WooF_worker_containers = new char*[container_count + 1];
    WooF_worker_containers[container_count] = NULL;
#ifdef DEBUG
    fprintf(stdout, "worker names\n");
#endif
    for (count = 0; count < container_count; ++count) {
        WooF_worker_containers[count] = (char*)malloc(1024);
        sprintf(WooF_worker_containers[count], "CSPOTWorker-%s-%x-%d", pathp + 1, WooFNameHash(WooF_namespace), count);
#ifdef DEBUG
        fprintf(stdout, "\t - %s\n", WooF_worker_containers[count]);
#endif
    }

    // kill any existing workers using CleanupDocker
    CleanUpContainers(WooF_worker_containers);

    // now create the new containers
#ifdef DEBUG
    fprintf(stdout, "WooFContainerLauncher started\n");
    fflush(stdout);
#endif

    tids = (pthread_t*)malloc(container_count * sizeof(pthread_t));
    if (tids == NULL) {
        fprintf(stderr, "WooFContainerLauncher no space\n");
        exit(1);
    }

    for (count = 0; count < container_count; count++) {
        /*
         * yield in case other threads need to complete
         */
#ifdef DEBUG
        fprintf(stdout, "WooFContainerLauncher: launch %d\n", count + 1);
        fflush(stdout);
#endif

        launch_string = (char*)malloc(1024 * 8);
        if (launch_string == NULL) {
            exit(1);
        }

        memset(launch_string, 0, 4096);

        port = WooFPortHash(WooF_namespace);

        // begin constructing the launch string
        sprintf(launch_string + strlen(launch_string),
                "docker run -t "
                "--rm " // option tells the container that it shuold remove itself when
                        // stopped
                "--name %s "
                "-e LD_LIBRARY_PATH=/usr/local/lib "
                "-e WOOFC_NAMESPACE=%s "
                "-e WOOFC_DIR=%s "
                "-e WOOF_NAME_ID=%lu "
                "-e WOOF_NAMELOG_NAME=%s "
                "-e WOOF_HOST_IP=%s ",
                WooF_worker_containers[count],
                WooF_namespace,
                pathp,
                Name_id,
                Namelog_name,
                Host_ip);

        if (count == 0) {
            sprintf(launch_string + strlen(launch_string), "-p %d:%d ", port, port);
        }

        sprintf(launch_string + strlen(launch_string),
                "-v %s:%s "
                "-v %s:/cspot-namelog "
                "cspot-docker-centos7 "
                "%s/%s ",
                WooF_dir,
                pathp,
                WooF_namelog_dir, /* all containers find namelog in /cspot-namelog */
                pathp,
                "woofc-container");

        if (count == 0) {
            sprintf(launch_string + strlen(launch_string), "-M ");
        }

#ifdef DEBUG
        fprintf(stdout, "\tcommand: '%s'\n", launch_string);
        fflush(stdout);
#endif

        std::cerr << launch_string << '\n';

        err = pthread_create(&tid, NULL, WooFDockerThread, (void*)launch_string);
        if (err < 0) {
            /* LOGGING
             * log thread create failure here
             */
            exit(1);
        }
        //		pthread_detach(tid);
        tids[count] = tid;
    }

    for (count = 0; count < container_count; count++) {
        pthread_join(tids[count], NULL);
    }

#ifdef DEBUG
    fprintf(stdout, "WooFContainerLauncher exiting\n");
    fflush(stdout);
#endif
    free(tids);
    pthread_exit(NULL);
}

#define ARGS "m:M:d:H:N:"
const char* Usage = "woofc-name-platform -d application woof directory\n\
\t-H directory for hostwide namelog\n\
\t-m min container count\n\
-t-M max container count\n\
-t-N namespace\n";

char putbuf0[1024];
char putbuf1[1024];
char putbuf2[1024];
char putbuf3[1024];
char putbuf4[1024];

/*
 * Linux fgets() doesn't appear to work in a signal handler
 */
char* SignalFgetS(char* buffer, int size, FILE* fd) {
    int i;
    char c;
    char* ptr;
    int count;

    i = 0;
    count = read(fileno(fd), &c, 1);
    while ((count > 0) && (i < size)) {
        buffer[i] = c;
        ptr = &(buffer[i]);
        if (c == '\n') {
            break;
        }
        i++;
        count = read(fileno(fd), &c, 1);
    }

    if (count > 0) {
        return (buffer);
    } else {
        return (NULL);
    }
}

void CleanUpContainers(char** names) {
    char** ptr = names;

    int name_lengths = 0;
    while (*ptr != NULL) {
        name_lengths += strlen(*ptr) + 2 /* account for the space we will add */;
        *ptr++;
    }
    ptr = names;

    char* command = new char[name_lengths + 512];
    command[0] = 0;
    strcpy(command, "docker rm -f ");
    while (*ptr != NULL) {
        sprintf(command + strlen(command), "%s ", *ptr);
        ptr++;
    }
    strcpy(command + strlen(command), "\n");
#ifdef DEBUG
    fprintf(stdout, "CleanUpDocker command: %s\n", command);
#endif
    system(command);
}

void CleanUpDocker(int signal, void* arg) {
    // simple guard, try to prevent two threads from making it in here at once
    // no real harm is done however if two threads do make it in other than
    // a possible double free, but the process is about to exit anyway
    if (WooF_worker_containers == NULL)
        return;

    char** names = WooF_worker_containers;
    WooF_worker_containers = NULL;
    CleanUpContainers(names);

    char** ptr = names;
    while (*ptr != NULL) {
        free(*ptr++);
    }
    free(names);
}

void sig_int_handler(int signal) {
    fprintf(stdout, "SIGINT caught\n");
    fflush(stdout);

    CleanUpDocker(0, NULL);
    exit(0);

    return;
}

void CatchSignals() {
    struct sigaction action;

    action.sa_handler = sig_int_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);

    return;
}

int main(int argc, char** argv, char** envp) {
    int c;
    int min_containers;
    int max_containers;
    char name_dir[2048];
    char name_space[2048];
    int err;

    //	signal(SIGINT,sig_int_handler);
    //	signal(SIGTERM,sig_int_handler);

    min_containers = 1;
    max_containers = 1;

    memset(name_dir, 0, sizeof(name_dir));
    memset(name_space, 0, sizeof(name_space));
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'm':
            min_containers = atoi(optarg);
            break;
        case 'M':
            max_containers = atoi(optarg);
            break;
        case 'H':
            strncpy(name_dir, optarg, sizeof(name_dir));
            break;
        case 'N':
            strncpy(name_space, optarg, sizeof(name_space));
            break;
        default:
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
    }

    if (min_containers < 0) {
        fprintf(stderr, "must specify valid number of min containers\n");
        fprintf(stderr, "%s", Usage);
        fflush(stderr);
        exit(1);
    }

    if (min_containers > max_containers) {
        fprintf(stderr, "min must be <= max containers\n");
        fprintf(stderr, "%s", Usage);
        fflush(stderr);
        exit(1);
    }

    /*
     * for czmq to work
     */
    strncpy(putbuf0,
            "LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib",
            strlen("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib"));
    putenv(putbuf0);

    if (name_dir[0] != 0) {
        sprintf(putbuf2, "WOOF_NAMELOG_DIR=%s", name_dir);
        putenv(putbuf2);
    }

    if (name_space[0] == 0) {
        getcwd(name_space, sizeof(name_space));
    }
    sprintf(putbuf1, "WOOFC_DIR=%s", name_space);
    putenv(putbuf1);
    sprintf(putbuf3, "WOOFC_NAMESPACE=%s", name_space);
    putenv(putbuf3);

#if IS_PLATFORM
    printf("starting platform listening to port %lu\n", WooFPortHash(name_space));
    fflush(stdout);
#endif

    //	fclose(stdin);

    CatchSignals();
#ifndef __APPLE__
    on_exit(CleanUpDocker, NULL);
#endif

    err = WooFLocalIP(Host_ip, sizeof(Host_ip));
    if (err < 0) {
        fprintf(stderr, "woofc-namespace-platform no local host IP found\n");
        exit(1);
    }
    sprintf(putbuf4, "WOOF_HOST_IP=%s", Host_ip);

    WooFHostInit(min_containers, max_containers);

    pthread_exit(NULL);

    return (0);
}

#endif
