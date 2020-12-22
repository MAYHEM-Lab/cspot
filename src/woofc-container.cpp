extern "C" {
#include "woofc-cache.h"
}


#include "debug.h"
#include "global.h"
#include "log.h"
#include "woofc-access.h"
#include "woofc-priv.h"
#include "woofc.h"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fmt/format.h>
#include <mutex>
#include <net.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#define ARGS "M"

namespace {
constexpr auto WOOF_CONTAINER_FORKERS = 15;
std::atomic<bool> should_exit;

sema ForkerThrottle;
std::atomic<int> Tcount;
} // namespace

void WooFShutdown(int) {
    int val;

    should_exit = true;
    while (sem_getvalue(&Name_log->tail_wait, &val) >= 0) {
        if (val > 0) {
            break;
        }
        V(&Name_log->tail_wait);
    }
}

void WooFForker();
void WooFReaper();

int WooFContainerInit() {
    struct timeval tm;
    int err;
    char* str;
    MIO* lmio;
    unsigned long name_id;
    int i;

    gettimeofday(&tm, NULL);
    srand48(tm.tv_sec + tm.tv_usec);

    str = getenv("WOOFC_NAMESPACE");
    DEBUG_FATAL_IF(!str, "WooFContainerInit: no namespace specified\n");
    cspot::globals::set_namespace(str);

    DEBUG_LOG("WooFContainerInit: namespace %s\n", WooF_namespace);

    str = getenv("WOOFC_DIR");
    DEBUG_FATAL_IF(!str, "WooFContainerInit: couldn't find WOOFC_DIR\n");

    DEBUG_FATAL_IF(strcmp(str, ".") == 0, "WooFWOOFC_DIR cannot be .\n");

    if (str[0] != '/') { /* not an absolute path name */
        getcwd(WooF_dir, sizeof(WooF_dir));
        if (str[0] == '.') {
            strncat(WooF_dir, &(str[1]), sizeof(WooF_dir) - strlen(WooF_dir));
        } else {
            strncat(WooF_dir, "/", sizeof(WooF_dir) - strlen(WooF_dir));
            strncat(WooF_dir, str, sizeof(WooF_dir) - strlen(WooF_dir));
        }
    } else {
        cspot::globals::set_dir(str);
    }

    DEBUG_FATAL_IF(strcmp(WooF_dir, "/") == 0, "WooFContainerInit: WOOFC_DIR can't be %s\n", WooF_dir);
    DEBUG_FATAL_IF(strlen(str) >= (sizeof(WooF_dir) - 1), "WooFContainerInit: %s too long for directory name\n", str);

    if (WooF_dir[strlen(WooF_dir) - 1] == '/') {
        WooF_dir[strlen(WooF_dir) - 1] = 0;
    }

    setenv("WOOFC_DIR", WooF_dir, 1);
    DEBUG_LOG("WooFContainerInit: WOOFC_DIR=%s\n", WooF_dir);

    str = getenv("WOOF_HOST_IP");
    DEBUG_FATAL_IF(!str, "WooFContainerInit: couldn't find local host IP\n");
    cspot::globals::set_host_ip(str);

    str = getenv("WOOF_NAME_ID");
    DEBUG_FATAL_IF(!str, "WooFContainerInit: couldn't find name id\n");
    name_id = strtoul(str, (char**)NULL, 10);

    str = getenv("WOOF_NAMELOG_NAME");
    DEBUG_FATAL_IF(!str, "WooFContainerInit: couldn't find namelog name\n");
    cspot::globals::set_namelog_name(str);

    err = mkdir(WooF_dir, 0600);
    if ((err < 0) && (errno != EEXIST)) {
        perror("WooFContainerInit");
        exit(1);
    }

    str = getenv("WOOF_NAMELOG_DIR");
    DEBUG_FATAL_IF(!str, "WooFContainerInit: couldn't find namelog dir\n");
    cspot::globals::set_namelog_dir(str);

    auto log_name = fmt::format("{}/{}", WooF_namelog_dir, Namelog_name);

    lmio = MIOReOpen(log_name.c_str());

    DEBUG_FATAL_IF(!lmio, "WooFOntainerInit: couldn't open mio for log %s\n", log_name.c_str());

    Name_log = (LOG*)MIOAddr(lmio);

    DEBUG_FATAL_IF(
        !Name_log, "WooFContainerInit: couldn't open log as %s, size %d\n", log_name.c_str(), DEFAULT_WOOF_LOG_SIZE);

    DEBUG_LOG("WooFContainerInit: log %s open\n", log_name.c_str());

    Name_id = name_id;

    InitSem(&ForkerThrottle, WOOF_CONTAINER_FORKERS);
    Tcount = WOOF_CONTAINER_FORKERS;

    for (i = 0; i < WOOF_CONTAINER_FORKERS; i++) {
        std::thread(WooFForker).detach();
    }
    std::thread(WooFReaper).detach();

    signal(SIGHUP, WooFShutdown);
    return (1);
}

void WooFExit() {
    should_exit = true;
    pthread_exit(NULL);
}

void WooFReaper() {
    int status;
    int i;
    struct timespec tspec;
    struct timeval then;
    struct timeval now;

    while (1) {
        gettimeofday(&now, NULL);
        for (i = 0; i < WOOF_CONTAINER_FORKERS; i++) {
            while (waitpid(-1, &status, WNOHANG) > 0) {
                /*
                 * Pd in Forker just before the fork
                 */
                then = now;
                gettimeofday(&now, NULL);
                V(&ForkerThrottle);

                Tcount++;
                DEBUG_LOG("Reaper: count after increment: %d\n", Tcount.load());
            }
        }
        if (then.tv_sec == now.tv_sec) {
            tspec.tv_sec = 0;
            tspec.tv_nsec = 5'000'000;
        } else {
            tspec.tv_sec = 1;
            tspec.tv_nsec = 0;
        }
        nanosleep(&tspec, NULL);
        then = now;
    }

    pthread_exit(NULL);
}

void WooFForker() {
    unsigned long last_seq_no = 0;
    unsigned long trigger_seq_no;
    unsigned long ls;
    unsigned long first;
    unsigned long firing;
    LOG* log_tail;
    EVENT* ev;
    EVENT* fev;
    int none;
    int firing_found;
    EVENT last_event{}; /* needed to understand if log tail has changed */
    int status;
    int pid;

    /*
     * wait for things to show up in the log
     */
    DEBUG_LOG("WooFForker: namespace: %s started\n", WooF_namespace);

    DEBUG_LOG("WooFForker: namespace: %s memset called\n", WooF_namespace);

    while (!should_exit) {
        DEBUG_LOG("WooFForker: namespace: %s caling P\n", WooF_namespace);
        P(&Name_log->tail_wait);
        DEBUG_LOG("WooFForker (%lu): namespace: %s awake\n", pthread_self(), WooF_namespace);

        if (should_exit) {
            break;
        }

        /*
         * must lock to sync on log tail
         */
        P(&Name_log->mutex);
        DEBUG_LOG("WooFForker (%lu): namespace: %s, in mutex, size: %lu, last: %lu\n",
                  pthread_self(),
                  WooF_namespace,
                  Name_log->size,
                  last_seq_no);
        log_tail = LogTail(Name_log, last_seq_no, Name_log->size);
        // log_tail = LogTail(Name_log, last_seq_no, Name_log->seq_no -
        // Name_log->last_trigger_seq_no);

        if (log_tail == NULL) {
            DEBUG_LOG("WooFForker: namespace: %s no tail, continuing\n", WooF_namespace);
            V(&Name_log->mutex);
            continue;
        }
        if (log_tail->head == log_tail->tail) {
            DEBUG_LOG("WooFForker (%lu): namespace: %s log tail empty, last: %lu continuing\n",
                      pthread_self(),
                      WooF_namespace,
                      last_seq_no);
            V(&Name_log->mutex);
            LogFree(log_tail);
            continue;
        }

        ev = (EVENT*)(static_cast<unsigned char*>(MIOAddr(log_tail->m_buf)) + sizeof(LOG));

        /*
         * find the first TRIGGER we haven't seen yet
         * skip triggers for other name spaces but try and wake them up
         */
        none = 0;
        first = log_tail->head;

        while (first != log_tail->tail) {
            /*
             * is this trigger in my namespace and unclaimed?
             */
            if ((ev[first].type == TRIGGER) &&
                (strncmp(ev[first].woofc_namespace, WooF_namespace, sizeof(ev[first].woofc_namespace)) == 0) &&
                (ev[first].seq_no > (unsigned long long)last_seq_no)) {
                /* now walk forward looking for FIRING */
                DEBUG_LOG("WooFForker: considering %s %llu\n", ev[first].woofc_namespace, ev[first].seq_no);
                firing = (first - 1);
                if (firing >= log_tail->size) {
                    firing = log_tail->size - 1;
                }
                trigger_seq_no = (unsigned long)ev[first].seq_no; /* for FIRING dependency */
                firing_found = 0;
                while (firing != log_tail->tail) {
                    if ((ev[firing].type == TRIGGER_FIRING) &&
                        (strncmp(ev[firing].woofc_namespace, WooF_namespace, sizeof(ev[firing].woofc_namespace)) ==
                         0) &&
                        (ev[firing].cause_seq_no == (unsigned long long)trigger_seq_no)) {
                        /* found FIRING */
                        firing_found = 1;
                        DEBUG_LOG(
                            "WooFForker: found firing for %s %llu\n", ev[first].woofc_namespace, ev[first].seq_no);
                        last_seq_no = (unsigned long)ev[first].seq_no;
                        break;
                    }
                    firing = firing - 1;
                    if (firing >= log_tail->size) {
                        firing = log_tail->size - 1;
                    }
                }
                if (firing_found == 0) {
                    DEBUG_LOG("WooFForker: no firing found for %s %llu\n", ev[first].woofc_namespace, ev[first].seq_no);
                    /* there is a TRIGGER with no FIRING */
                    break;
                }
            }

            /*
             * if this is a trigger belonging to another namespace, try and wake
             * its container(s)
             *
             * Note that this may cause spurious wake ups since this container
             * can't tell if the others have seen this trigger yet
             */

            auto illegal = (ev[first].type == TRIGGER) && (memcmp(&last_event, &ev[first], sizeof(last_event)) != 0) &&
                           (strncmp(ev[first].woofc_namespace, WooF_namespace, sizeof(ev[first].woofc_namespace)) != 0);
            DEBUG_FATAL_IF(illegal,
                           "WooFForker: namespace: %s found trigger for evns: %s, name: %s, "
                           "first: %lu, head: %lu, tail: %lu\n",
                           WooF_namespace,
                           ev[first].woofc_namespace,
                           ev[first].woofc_name,
                           first,
                           log_tail->head,
                           log_tail->tail);

            // TODO: only go back to latest triggered
            first = (first - 1);
            if (first >= log_tail->size) {
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
            DEBUG_LOG("WooFForker log tail empty, continuing\n");
            V(&Name_log->mutex);
            LogFree(log_tail);
            continue;
        }
        /*
         * otherwise, fire this event
         */

        DEBUG_LOG("WooFForker (%lu): namespace: %s accepted and firing woof: %s handler: "
                  "%s woof_seq_no: %lu log_seq_no: %llu\n",
                  pthread_self(),
                  WooF_namespace,
                  ev[first].woofc_name,
                  ev[first].woofc_handler,
                  ev[first].woofc_seq_no,
                  ev[first].seq_no);

        // Name_log->last_trigger_seq_no = (unsigned long long)trigger_seq_no;
        /*
         * before dropping mutex, log a FIRING record
         */
        fev = EventCreate(TRIGGER_FIRING, Name_id);
        if (fev == NULL) {
            fprintf(stderr, "WooFForker: couldn't create TRIGGER_FIRING record\n");
            V(&Name_log->mutex);
            exit(1);
        }
        fev->cause_host = Name_id;
        fev->cause_seq_no = (unsigned long long)trigger_seq_no;
        memset(fev->woofc_namespace, 0, sizeof(fev->woofc_namespace));
        strncpy(fev->woofc_namespace, WooF_namespace, sizeof(fev->woofc_namespace));
        DEBUG_LOG("WooFForker: logging TRIGGER_FIRING for %s %llu\n", ev[first].woofc_namespace, ev[first].seq_no);
        /*
         * must be LogAdd() call since inside of critical section
         */
        ls = LogEventNoLock(Name_log, fev);
        if (ls == 0) {
            fprintf(stderr, "WooFForker: couldn't log event to log\n");
            fflush(stderr);
            EventFree(fev);
            V(&Name_log->mutex);
            exit(1);
        }
        EventFree(fev);

        /*
         * drop the mutex now that this TRIGGER is claimed
         */
        V(&Name_log->mutex);
        DEBUG_LOG("WooFForker: namespace: %s out of mutex with log tail\n", WooF_namespace);


        /*
         * here, we need to fork a new process for the handler
         */


        /*
         * block here not to overload the machine
         */
        DEBUG_LOG("Forker calling P with Tcount %d\n", Tcount.load());

        P(&ForkerThrottle);
        Tcount--;
        DEBUG_LOG("Forker awake, after decrement %d\n", Tcount.load());

        pid = fork();
        if (pid == 0) {
            /*
             * I am the child.  I need the read end but not the write end
             */

            close(0); /* so shepherd knows there is no pipe */

            auto wf = WooFOpen(ev[first].woofc_name);

            DEBUG_FATAL_IF(!wf, "WooFForker: open failed for WooF at %s, %lu %lu\n",
                           ev[first].woofc_name,
                           ev[first].woofc_element_size,
                           ev[first].woofc_history_size);

            std::vector<std::string> env;

            env.emplace_back(fmt::format("WOOFC_NAMESPACE={}", WooF_namespace));
            env.emplace_back(fmt::format("WOOFC_DIR={}", WooF_dir));
            env.emplace_back(fmt::format("WOOF_HOST_IP={}", Host_ip));
            env.emplace_back(fmt::format("WOOF_SHEPHERD_NAME={}", wf->shared->filename));
            env.emplace_back(fmt::format("WOOF_SHEPHERD_NDX={}", ev[first].woofc_ndx));
            env.emplace_back(fmt::format("WOOF_SHEPHERD_SEQNO={}", ev[first].woofc_seq_no));
            env.emplace_back(fmt::format("WOOF_NAME_ID={}", Name_id));
            env.emplace_back(fmt::format("WOOF_NAMELOG_DIR={}", WooF_namelog_dir));
            env.emplace_back(fmt::format("WOOF_NAMELOG_NAME={}", Namelog_name));
            env.emplace_back(fmt::format("WOOF_NAMELOG_SEQNO={}", ev[first].seq_no));
            env.emplace_back("LD_LIBRARY_PATH=/usr/local/lib");

            auto binary = fmt::format("{}/{}", WooF_dir, ev[first].woofc_handler);

            char* earg[2];
            earg[0] = const_cast<char*>(binary.c_str());
            earg[1] = nullptr;

            WooFFree(wf);

            std::vector<char*> eenvp(env.size());
            std::transform(
                env.begin(), env.end(), eenvp.begin(), [](auto& str) { return const_cast<char*>(str.c_str()); });
            eenvp.push_back(nullptr);

            execve(binary.c_str(), earg, eenvp.data());

            fprintf(stderr, "WooFForker: execve of %s failed\n", binary.c_str());
            exit(1);
        } else if (pid < 0) {
            //            fprintf(stderr,
            //                    "WooFForker: fork failed for %s/%s in %s/%s\n",
            //                    WooF_dir,
            //                    ev[first].woofc_handler,
            //                    WooF_namespace,
            //                    wf->shared->filename);
            //            fflush(stderr);
            should_exit = true;
        } else { /* parent */

            /*
             * remember its sequence number for next time
             */
            last_seq_no = (unsigned long)ev[first].seq_no; /* log seq_no */
            DEBUG_LOG("WooFForker: namespace: %s seq_no: %llu, handler: %s\n",
                      WooF_namespace,
                      ev[first].seq_no,
                      ev[first].woofc_handler);
            LogFree(log_tail);
        }

        while (waitpid(-1, &status, WNOHANG) > 0) {
            V(&ForkerThrottle);
            Tcount++;
            DEBUG_LOG("Parent: count after increment: %d\n", Tcount.load());
        }
    }

    fprintf(stderr, "WooFForker namespace: %s exiting\n", WooF_namespace);
    fflush(stderr);
}

int main(int argc, char** argv) {
    int message_server = 0;

    cspot::set_active_backend(cspot::get_backend_with_name("zmq"));

    WooFContainerInit();

    /*
     * check c != 255 for Raspberry Pi
     */
    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF && c != 255) {
        switch (c) {
        case 'M':
            message_server = 1;
            break;
        }
    }

    DEBUG_LOG("woofc-container: about to start message server with namespace %s\n", WooF_namespace);

    /*
     * start the msg server for this container
     *
     * for now, this doesn't ever return
     */
    if (message_server) {
        fprintf(stdout, "woofc-container: started message server\n");
        [[maybe_unused]] auto err = WooFMsgServer(WooF_namespace);
    } else {
        fprintf(stdout, "woofc-container: message server disabled. not listening.\n");
    }
    fflush(stdout);
}
