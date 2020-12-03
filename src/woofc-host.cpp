extern "C" {
#include "log.h"
#include "woofc.h"
}

#include "debug.h"
#include "global.h"
#include "woofc-access.h"
#include "woofc-priv.h"

#include <atomic>
#include <errno.h>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace {
std::vector<std::string> worker_containers;
std::atomic<bool> should_exit;
}

struct cont_arg_stc {
    int container_count;
};

typedef struct cont_arg_stc CA;

void WooFShutdown(int sig) {
    int val;

    should_exit = true;
    while (sem_getvalue(&Name_log->tail_wait, &val) >= 0) {
        if (val > 0) {
            break;
        }
        V(&Name_log->tail_wait);
    }
}

namespace {
const char* from_env_or(const char* env_key, const char* or_) {
    auto env = getenv(env_key);
    if (!env) {
        return or_;
    }
    return env;
}
} // namespace

int WooFInit() {
    struct timeval tm;
    int err;
    char* str;
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

    setenv("WOOFC_DIR", WooF_dir, 1);

    DEBUG_LOG("WooFInit: WooF_dir: %s\n", WooF_dir);

    /*
     * note that the container will always have WOOFC_NAMESPACE specified by the launcher
     */
    cspot::globals::set_namespace(from_env_or("WOOFC_NAMESPACE", WooF_dir));
    DEBUG_LOG("WooFInit: namespace: %s\n", WooF_namespace);

    cspot::globals::set_namelog_dir(from_env_or("WOOF_NAMELOG_DIR", WooF_dir));
    DEBUG_LOG("WooFInit: namelog dir: %s\n", WooF_namelog_dir);

    err = mkdir(WooF_dir, 0600);
    if ((err < 0) && (errno != EEXIST)) {
        perror("WooFInit");
        return (-1);
    }

    name_id = WooFNameHash(WooF_namelog_dir);
    sprintf(Namelog_name, "cspot-log.%20.20lu", name_id);

    char log_name[2048];
    memset(log_name, 0, sizeof(log_name));
    sprintf(log_name, "%s/%s", WooF_namelog_dir, Namelog_name);

    DEBUG_LOG("WooFInit: Name log at %s with name %s\n", log_name, Namelog_name);

#ifndef IS_PLATFORM
    MIO *lmio = MIOReOpen(log_name);
    if (lmio == NULL) {
        fprintf(stderr, "WooFInit: name %s (%s) log not initialized.\n", log_name, WooF_dir);
        fflush(stderr);
        exit(1);
    }
    Name_log = static_cast<LOG*>(MIOAddr(lmio));

#endif
    Name_id = name_id;

    return 1;
}

#ifdef IS_PLATFORM
void WooFContainerLauncher(std::unique_ptr<CA>);

void CatchSignals();

void CleanUpDocker(int, void*);
void CleanUpContainers(const std::vector<std::string>& names);

static int WooFHostInit(int min_containers, int max_containers) {
    WooFInit();

    auto log_name = fmt::format("{}/{}", WooF_namelog_dir, Namelog_name);
    Name_log = LogCreate(log_name.c_str(), Name_id, DEFAULT_WOOF_LOG_SIZE);
    DEBUG_FATAL_IF(Name_log == nullptr,
                   "WooFInit: couldn't create name log as %s, size %d\n",
                   log_name.c_str(),
                   DEFAULT_WOOF_LOG_SIZE);

    DEBUG_LOG("WooFHostInit: created %s\n", log_name.c_str());

    auto ca = std::make_unique<CA>();
    ca->container_count = min_containers;

    auto thread = std::thread(WooFContainerLauncher, std::move(ca));

    /*
     * sleep for 10 years
     *
     * calling pthread_join here causes pthreads to intercept the SIGINT signal handler
     * (which sounds like a bug in Linux).  Sleeping forever, however, allows SIGINT to
     * be caught which then triggers a clean up of the docker container
     */
    sleep(86400 * 365 * 10);
    thread.join();

    exit(0);
}

void WooFExit() {
    should_exit = true;
    pthread_exit(NULL);
}

/*
 * FIX ME: it would be better if the TRIGGER seq_no gets
 * retired after the launch is successful  Right now, the last_seq_no
 * in the launcher is set before the launch actually happens
 * which means a failure will not be retried
 */
void WooFDockerThread(std::string launch_string) {
    DEBUG_LOG("LAUNCH: %s\n", launch_string.c_str());

    system(launch_string.c_str());

    DEBUG_LOG("DONE: %s\n", launch_string.c_str());
}

void WooFContainerLauncher(std::unique_ptr<CA> ca) {
    int container_count = ca->container_count;
    ca.reset();

    /*
     * find the last directory in the path
     */
    std::string pathp = WooF_dir;
    if (pathp.empty()) {
        fprintf(stderr, "couldn't find leaf dir in %s\n", WooF_dir);
        exit(1);
    }

    // build the names for the workers to spawn
    DEBUG_LOG("worker names\n");
    for (int count = 0; count < container_count; ++count) {
        auto name_path_part = pathp;
        std::replace(name_path_part.begin(), name_path_part.end(), '/', '_');
        DEBUG_LOG(name_path_part.c_str());

        worker_containers.emplace_back(
            fmt::format("CSPOTWorker-{}-{:x}-{}", name_path_part, WooFNameHash(WooF_namespace), count));
        DEBUG_LOG("\t - %s\n", worker_containers[count].c_str());
    }

    // kill any existing workers using CleanupDocker
    CleanUpContainers(worker_containers);

    // now create the new containers
    DEBUG_LOG("WooFContainerLauncher started\n");

    std::vector<std::thread> launch_threads;

    for (int count = 0; count < container_count; count++) {
        DEBUG_LOG("WooFContainerLauncher: launch %d\n", count + 1);

        auto launch_string = (char*)malloc(1024 * 8);
        if (launch_string == NULL) {
            exit(1);
        }

        memset(launch_string, 0, 1024 * 8);

        auto port = WooFPortHash(WooF_namespace);

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
                worker_containers[count].c_str(),
                WooF_namespace,
                pathp.data(),
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
                pathp.data(),
                WooF_namelog_dir, /* all containers find namelog in /cspot-namelog */
                pathp.data(),
                "woofc-container");

        if (count == 0) {
            sprintf(launch_string + strlen(launch_string), "-M ");
        }

        DEBUG_LOG("\tcommand: '%s'\n", launch_string);

        std::cerr << launch_string << '\n';

        launch_threads.emplace_back([=] { WooFDockerThread(launch_string); });
    }

    for (auto& thread : launch_threads) {
        thread.join();
    }

    DEBUG_LOG("WooFContainerLauncher exiting\n");
}

#define ARGS "m:M:d:H:N:"
const char* Usage = "woofc-name-platform -d application woof directory\n\
\t-H directory for hostwide namelog\n\
\t-m min container count\n\
-t-M max container count\n\
-t-N namespace\n";

void CleanUpContainers(const std::vector<std::string>& names) {
    auto command = fmt::format("docker rm -f {}\n", fmt::join(names, ", "));
    DEBUG_LOG("CleanUpDocker command: %s\n", command.c_str());
    system(command.c_str());
}

void CleanUpDocker([[maybe_unused]] int signal, [[maybe_unused]] void* arg) {
    // simple guard, try to prevent two threads from making it in here at once
    // no real harm is done however if two threads do make it in other than
    // a possible double free, but the process is about to exit anyway
    if (worker_containers.empty())
        return;

    CleanUpContainers(worker_containers);
    worker_containers.clear();
}

void sig_int_handler(int) {
    fprintf(stdout, "SIGINT caught\n");
    fflush(stdout);

    CleanUpDocker(0, nullptr);
    exit(0);
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
}

int main(int argc, char** argv) {
    int c;
    int min_containers;
    int max_containers;
    char name_dir[2048];
    char name_space[2048];
    int err;

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

    setenv("LD_LIBRARY_PATH", "$LD_LIBRARY_PATH:/usr/local/lib", 1);

    if (name_dir[0] != 0) {
        setenv("WOOF_NAMELOG_DIR", name_dir, 1);
    }

    if (name_space[0] == 0) {
        getcwd(name_space, sizeof(name_space));
    }

    setenv("WOOFC_DIR", name_space, 1);
    setenv("WOOFC_NAMESPACE", name_space, 1);

#if IS_PLATFORM
    DEBUG_LOG("starting platform listening to port %lu\n", WooFPortHash(name_space));
#endif

    //	fclose(stdin);

    CatchSignals();
#ifndef __APPLE__
    on_exit(CleanUpDocker, NULL);
#endif

    err = WooFLocalIP(Host_ip, sizeof(Host_ip));
    DEBUG_FATAL_IF(err < 0, "woofc-namespace-platform no local host IP found\n");

    setenv("WOOF_HOST_IP", Host_ip, 1);

    WooFHostInit(min_containers, max_containers);

    pthread_exit(nullptr);
}

#endif
