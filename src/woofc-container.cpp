extern "C" {
#include "log.h"
#include "woofc-access.h"
#include "woofc-cache.h"
#include "woofc.h"
}

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fmt/format.h>
#include <mutex>
#include <thread>
#include <unistd.h>

char WooF_dir[2048];
char WooF_namespace[2048];
char WooF_namelog_dir[2048];
char Host_ip[25];
char Namelog_name[2048];
unsigned long Name_id;
LOG* Name_log;

#define ARGS "M"

namespace {
constexpr auto WOOF_CONTAINER_FORKERS = 15;
int WooFDone;

sema ForkerThrottle;
std::mutex Tlock;
int Tcount;
} // namespace

void WooFShutdown(int) {
    int val;

    WooFDone = 1;
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
    char putbuf[25];
    char* str;
    MIO* lmio;
    unsigned long name_id;
    int i;

    gettimeofday(&tm, NULL);
    srand48(tm.tv_sec + tm.tv_usec);

    str = getenv("WOOFC_NAMESPACE");
    if (str == NULL) {
        fprintf(stderr, "WooFContainerInit: no namespace specified\n");
        exit(1);
    }
    strncpy(WooF_namespace, str, sizeof(WooF_namespace));
#ifdef DEBUG
    fprintf(stdout, "WooFContainerInit: namespace %s\n", WooF_namespace);
    fflush(stdout);
#endif

    str = getenv("WOOFC_DIR");
    if (str == NULL) {
        fprintf(stderr, "WooFContainerInit: couldn't find WOOFC_DIR\n");
        exit(1);
    }

    if (strcmp(str, ".") == 0) {
        fprintf(stderr, "WOOFC_DIR cannot be .\n");
        fflush(stderr);
        exit(1);
    }

    if (str[0] != '/') { /* not an absolute path name */
        getcwd(WooF_dir, sizeof(WooF_dir));
        if (str[0] == '.') {
            strncat(WooF_dir, &(str[1]), sizeof(WooF_dir) - strlen(WooF_dir));
        } else {
            strncat(WooF_dir, "/", sizeof(WooF_dir) - strlen(WooF_dir));
            strncat(WooF_dir, str, sizeof(WooF_dir) - strlen(WooF_dir));
        }
    } else {
        strncpy(WooF_dir, str, sizeof(WooF_dir));
    }

    if (strcmp(WooF_dir, "/") == 0) {
        fprintf(stderr, "WooFContainerInit: WOOFC_DIR can't be %s\n", WooF_dir);
        exit(1);
    }

    if (strlen(str) >= (sizeof(WooF_dir) - 1)) {
        fprintf(stderr, "WooFContainerInit: %s too long for directory name\n", str);
        exit(1);
    }

    if (WooF_dir[strlen(WooF_dir) - 1] == '/') {
        WooF_dir[strlen(WooF_dir) - 1] = 0;
    }

    fmt::format_to(putbuf, "WOOFC_DIR={}", WooF_dir);
    putenv(putbuf);
#ifdef DEBUG
    fprintf(stdout, "WooFContainerInit: %s\n", putbuf);
    fflush(stdout);
#endif

    str = getenv("WOOF_HOST_IP");
    if (str == NULL) {
        fprintf(stderr, "WooFContainerInit: couldn't find local host IP\n");
        exit(1);
    }
    strncpy(Host_ip, str, sizeof(Host_ip));

    str = getenv("WOOF_NAME_ID");
    if (str == NULL) {
        fprintf(stderr, "WooFContainerInit: couldn't find name id\n");
        exit(1);
    }
    name_id = strtoul(str, (char**)NULL, 10);

    str = getenv("WOOF_NAMELOG_NAME");
    if (str == NULL) {
        fprintf(stderr, "WooFContainerInit: couldn't find namelog name\n");
        exit(1);
    }

    strncpy(Namelog_name, str, sizeof(Namelog_name));

    err = mkdir(WooF_dir, 0600);
    if ((err < 0) && (errno != EEXIST)) {
        perror("WooFContainerInit");
        exit(1);
    }

    strncpy(WooF_namelog_dir, "/cspot-namelog", sizeof(WooF_namelog_dir));

    auto log_name = fmt::format("{}/{}", WooF_namelog_dir, Namelog_name);

    lmio = MIOReOpen(log_name.c_str());
    if (lmio == NULL) {
        fprintf(stderr, "WooFOntainerInit: couldn't open mio for log %s\n", log_name.c_str());
        fflush(stderr);
        exit(1);
    }
    Name_log = (LOG*)MIOAddr(lmio);

    if (Name_log == NULL) {
        fprintf(
            stderr, "WooFContainerInit: couldn't open log as %s, size %d\n", log_name.c_str(), DEFAULT_WOOF_LOG_SIZE);
        fflush(stderr);
        exit(1);
    }

#ifdef DEBUG
    printf("WooFContainerInit: log %s open\n", log_name);
    fflush(stdout);
#endif

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
    WooFDone = 1;
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

                std::lock_guard lg{Tlock};
                Tcount++;
#ifdef DEBUG
                printf("Reaper: count after increment: %d\n", Tcount);
                fflush(stdout);
#endif
            }
        }
        if (then.tv_sec == now.tv_sec) {
            tspec.tv_sec = 0;
            tspec.tv_nsec = 5000000;
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
    char* pathp;
    char woof_shepherd_dir[2048];
    int none;
    int firing_found;
    EVENT last_event{}; /* needed to understand if log tail has changed */
    int status;
    int pid;

    /*
     * wait for things to show up in the log
     */
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
        fprintf(stdout, "WooFForker (%lu): namespace: %s awake\n", pthread_self(), WooF_namespace);
        fflush(stdout);
#endif

        //		pthread_yield();

        if (WooFDone == 1) {
            break;
        }

        /*
         * must lock to sync on log tail
         */
        P(&Name_log->mutex);
#ifdef DEBUG
        fprintf(stdout,
                "WooFForker (%lu): namespace: %s, in mutex, size: %lu, last: %lu\n",
                pthread_self(),
                WooF_namespace,
                Name_log->size,
                last_seq_no);
        fflush(stdout);
#endif
        log_tail = LogTail(Name_log, last_seq_no, Name_log->size);
        // log_tail = LogTail(Name_log, last_seq_no, Name_log->seq_no -
        // Name_log->last_trigger_seq_no);

        if (log_tail == NULL) {
#ifdef DEBUG
            fprintf(stdout, "WooFForker: namespace: %s no tail, continuing\n", WooF_namespace);
            fflush(stdout);
#endif
            V(&Name_log->mutex);
            continue;
        }
        if (log_tail->head == log_tail->tail) {
#ifdef DEBUG
            fprintf(stdout,
                    "WooFForker (%lu): namespace: %s log tail empty, last: %lu continuing\n",
                    pthread_self(),
                    WooF_namespace,
                    last_seq_no);
            fflush(stdout);
#endif
            V(&Name_log->mutex);
            LogFree(log_tail);
            continue;
        }

        ev = (EVENT*)(static_cast<char*>(MIOAddr(log_tail->m_buf)) + sizeof(LOG));

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
#ifdef DEBUG
                printf("WooFForker: considering %s %llu\n", ev[first].namespace, ev[first].seq_no);
                fflush(stdout);
#endif
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
#ifdef DEBUG
                        printf("WooFForker: found firing for %s %llu\n", ev[first].namespace, ev[first].seq_no);
                        fflush(stdout);
#endif
                        last_seq_no = (unsigned long)ev[first].seq_no;
                        break;
                    }
                    firing = firing - 1;
                    if (firing >= log_tail->size) {
                        firing = log_tail->size - 1;
                    }
                }
                if (firing_found == 0) {
#ifdef DEBUG
                    printf("WooFForker: no firing found for %s %llu\n", ev[first].namespace, ev[first].seq_no);
                    fflush(stdout);
#endif
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
            if ((ev[first].type == TRIGGER) && (memcmp(&last_event, &ev[first], sizeof(last_event)) != 0) &&
                (strncmp(ev[first].woofc_namespace, WooF_namespace, sizeof(ev[first].woofc_namespace)) != 0)) {
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
            }
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
#ifdef DEBUG
            fprintf(stdout, "WooFForker log tail empty, continuing\n");
            fflush(stdout);
#endif
            V(&Name_log->mutex);
            LogFree(log_tail);
            continue;
        }
        /*
         * otherwise, fire this event
         */

#ifdef DEBUG
        fprintf(stdout,
                "WooFForker (%lu): namespace: %s accepted and firing woof: %s handler: "
                "%s woof_seq_no: %lu log_seq_no: %llu\n",
                pthread_self(),
                WooF_namespace,
                ev[first].woofc_name,
                ev[first].woofc_handler,
                ev[first].woofc_seq_no,
                ev[first].seq_no);
        fflush(stdout);
#endif

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
#ifdef DEBUG
        printf("WooFForker: logging TRIGGER_FIRING for %s %llu\n", ev[first].namespace, ev[first].seq_no);
        fflush(stdout);
#endif
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
#ifdef DEBUG
        fprintf(stdout, "WooFForker: namespace: %s out of mutex with log tail\n", WooF_namespace);
        fflush(stdout);
#endif


        /*
         * here, we need to fork a new process for the handler
         */


        /*
         * block here not to overload the machine
         */
        {
            std::lock_guard lg{Tlock};
#ifdef DEBUG
            printf("Forker calling P with Tcount %d\n", Tcount);
            fflush(stdout);
#endif
        }

        {
            P(&ForkerThrottle);
            std::lock_guard lg{Tlock};
            Tcount--;
#ifdef DEBUG
            printf("Forker awake, after decrement %d\n", Tcount);
            fflush(stdout);
#endif
        }

        pid = fork();
        if (pid == 0) {
            /*
             * I am the child.  I need the read end but not the write end
             */

            close(0); /* so shepherd knows there is no pipe */

            auto wf = WooFOpen(ev[first].woofc_name);

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
            env.emplace_back(fmt::format("WOOF_NAMELOG_DIR={}", WooF_namelog_dir));

            auto binary = fmt::format("{}/{}", WooF_dir, ev[first].woofc_handler);

            char* earg[2];
            earg[0] = const_cast<char*>(binary.c_str());
            earg[1] = nullptr;

            WooFFree(wf);

            std::vector<char*> eenvp(env.size());
            std::transform(
                env.begin(), env.end(), eenvp.begin(), [](auto& str) { return const_cast<char*>(str.c_str()); });

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
            WooFDone = 1;
        } else { /* parent */

            /*
             * remember its sequence number for next time
             */
            last_seq_no = (unsigned long)ev[first].seq_no; /* log seq_no */
#ifdef DEBUG
            fprintf(stdout,
                    "WooFForker: namespace: %s seq_no: %llu, handler: %s\n",
                    WooF_namespace,
                    ev[first].seq_no,
                    ev[first].woofc_handler);
            fflush(stdout);
#endif
            LogFree(log_tail);
        }

        while (waitpid(-1, &status, WNOHANG) > 0) {
            V(&ForkerThrottle);
            std::lock_guard lg{Tlock};
            Tcount++;
#ifdef DEBUG
            printf("Parent: count after increment: %d\n", Tcount);
            fflush(stdout);
#endif
        }
    }

    fprintf(stderr, "WooFForker namespace: %s exiting\n", WooF_namespace);
    fflush(stderr);
}

int main(int argc, char** argv) {
    int message_server = 0;

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

#ifdef DEBUG
    printf("woofc-container: about to start message server with namespace %s\n", WooF_namespace);
    fflush(stdout);
#endif

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
