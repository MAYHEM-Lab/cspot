#define DEBUG

#include "log.h"
#include "mio.h"
#include "repair.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ARGS "DH:F:W:S:O:"
char* Usage = "woof-repair -H ip1:port1,ip2:port2... -F glog_filename -W woof -S "
              "seq_no_1,seq_no_2,... -O guide_filename\n";

extern unsigned long Name_id;

int Repair(GLOG* glog, char* namespace, unsigned long host, char* wf_name);
int RepairRO(GLOG* glog, char* namespace, unsigned long host, char* wf_name);
void Guide(GLOG* glog, FILE* fd);

int dryrun;

unsigned long hash(char* namespace) {
    unsigned long h = 5381;
    unsigned long a = 33;
    unsigned long i;

    for (i = 0; i < strlen(namespace); i++) {
        h = ((h * a) + namespace[i]); /* no mod p due to wrap */
    }
    return (h);
}

int main(int argc, char** argv) {
    int i;
    int c;
    int err;
    char* ptr;
    int num_hosts;
    int num_seq_no;
    unsigned long log_size;
    unsigned long total_log_size;
    unsigned long glog_size;
    unsigned long total_events;
    MIO** mio;
    LOG** log;
    GLOG* glog;
    char** endpoint;
    RB* seq_no;
    char hosts[4096];
    char filename[4096];
    char woof[4096];
    char namespace[4096];
    char woof_name[4096];
    char seq_nos[4096];
    char guide_file[4096];
    unsigned long woof_name_id;
    RB* root;
    RB* casualty;
    RB* progress;
    RB* rb;
    int count_root;
    int count_casualty;
    int count_progress;
    char** woof_root;
    char** woof_casualty;
    char** woof_progress;
    unsigned long wf_host;
    char* wf_name;
    RB* asker;
    RB* mapping_count;
    FILE* guide_fd;
    struct timeval t1, t2;
    double elapsedTime;

#ifdef REPAIR
    dryrun = 0;
    memset(hosts, 0, sizeof(hosts));
    memset(filename, 0, sizeof(filename));
    memset(woof, 0, sizeof(woof));
    memset(namespace, 0, sizeof(namespace));
    memset(seq_nos, 0, sizeof(seq_nos));
    memset(guide_file, 0, sizeof(guide_file));

    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'D':
            dryrun = 1;
            break;
        case 'H':
            strncpy(hosts, optarg, sizeof(hosts));
            break;
        case 'F':
            strncpy(filename, optarg, sizeof(filename));
            break;
        case 'W':
            strncpy(woof, optarg, sizeof(woof));
            break;
        case 'S':
            strncpy(seq_nos, optarg, sizeof(seq_nos));
            break;
        case 'O':
            strncpy(guide_file, optarg, sizeof(guide_file));
            break;
        default:
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
    }

    if (filename[0] == 0 || hosts[0] == 0 || seq_nos[0] == 0 || guide_file[0] == 0) {
        fprintf(stderr, "%s", Usage);
        fflush(stderr);
        exit(1);
    }

    guide_fd = fopen(guide_file, "w");
    if (guide_fd == NULL) {
        fprintf(stderr, "couldn't open guide_file %s\n", guide_file);
        fflush(stderr);
        exit(1);
    }

    // parse hosts
    num_hosts = 1;
    for (i = 0; hosts[i] != '\0'; i++) {
        if (hosts[i] == ',') {
            num_hosts++;
        }
    }
    endpoint = malloc(num_hosts * sizeof(char*));
    for (i = 0; i < num_hosts; i++) {
        endpoint[i] = malloc(128 * sizeof(char));
        memset(endpoint[i], 0, sizeof(endpoint[i]));
    }

    i = 0;
    ptr = strtok(hosts, ",");
    while (ptr != NULL) {
        sprintf(endpoint[i], ">tcp://%s", ptr);
        ptr = strtok(NULL, ",");
        i++;
    }
#ifdef DEBUG
    printf("num_hosts: %d\n", num_hosts);
    fflush(stdout);
#endif

    // parse seq_no
    num_seq_no = 1;
    for (i = 0; seq_nos[i] != '\0'; i++) {
        if (seq_nos[i] == ',') {
            num_seq_no++;
        }
    }
    seq_no = RBInitI64();
    if (seq_no == NULL) {
        fprintf(stderr, "couldn't initialize seq_no\n");
        fflush(stderr);
        for (i = 0; i < num_hosts; i++) {
            free(endpoint[i]);
        }
        free(endpoint);
        exit(1);
    }

    i = 0;
    ptr = strtok(seq_nos, ",");
    while (ptr != NULL) {
        RBInsertI64(seq_no, (int64_t)strtoul(ptr, (char**)NULL, 10), (Hval)0);
        ptr = strtok(NULL, ",");
        i++;
    }
#ifdef DEBUG
    printf("num_seq_no: %d\n", num_seq_no);
    fflush(stdout);
#endif

    WooFInit();

    total_log_size = 0;
    glog_size = 0;
    mio = malloc(num_hosts * sizeof(MIO*));
    log = malloc(num_hosts * sizeof(LOG*));
#ifdef DEBUG
    printf("start pulling log from hosts\n");
    fflush(stdout);
    gettimeofday(&t1, NULL);
#endif
    for (i = 0; i < num_hosts; i++) {
#ifdef DEBUG
        printf("pulling log from host %d\n", i + 1);
        fflush(stdout);
#endif
        log_size = LogGetRemoteSize(endpoint[i]);
        total_log_size += log_size;
        if (log_size < 0) {
            fprintf(stderr, "couldn't get remote log size from %s\n", endpoint[i]);
            fflush(stderr);
            exit(1);
        }
        mio[i] = MIOMalloc(log_size);
        log[i] = (LOG*)MIOAddr(mio[i]);
        err = LogGetRemote(log[i], mio[i], endpoint[i]);
        if (err < 0) {
            fprintf(stderr, "couldn't get remote log from %s\n", endpoint[i]);
            fflush(stderr);
            exit(1);
        }
        glog_size += log[i]->size;
        // LogPrint(stdout, log[i]);
        // fflush(stdout);
#ifdef DEBUG
        printf("pulled log from host %d, size %lu bytes\n", i + 1, log_size);
        fflush(stdout);
#endif
    }
#ifdef DEBUG
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf("finished pulling log from hosts: %f ms, %lu bytes in total\n",
           elapsedTime,
           total_log_size);
    fflush(stdout);
#endif

#ifdef DEBUG
    printf("start merging logs\n");
    fflush(stdout);
    gettimeofday(&t1, NULL);
#endif

    total_events = 0;
#ifdef DEBUG
    printf("creating GLog with size %lu...\n", glog_size);
    fflush(stdout);
#endif
    glog = GLogCreate(filename, Name_id, glog_size);
    if (glog == NULL) {
        fprintf(stderr, "couldn't create global log\n");
        fflush(stderr);
        exit(1);
    }
#ifdef DEBUG
    printf("GLog created\n", glog_size);
    fflush(stdout);
#endif

    for (i = 0; i < num_hosts; i++) {
#ifdef DEBUG
        printf("importing log from host %d...\n", i + 1);
        fflush(stdout);
#endif
        err = ImportLogTail(glog, log[i]);
        if (err < 0) {
            fprintf(stderr, "couldn't import log tail from %s\n", endpoint[i]);
            fflush(stderr);
            exit(1);
        }
        total_events += log[i]->seq_no - 1;
#ifdef DEBUG
        printf("imported log from host %d, %lu events\n", i + 1, log[i]->seq_no - 1);
        fflush(stdout);
#endif
    }
#ifdef DEBUG
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf(
        "finished merging logs: %f ms, %lu events in total\n", elapsedTime, total_events);
    fflush(stdout);
#endif

#ifdef DEBUG
    printf("Global log:\n");
    GLogPrint(stdout, glog);
    fflush(stdout);
#endif

    err = WooFNameSpaceFromURI(woof, namespace, sizeof(namespace));
    if (err < 0) {
        fprintf(stderr, "woof: %s no name space\n", woof);
        fflush(stderr);
        return (-1);
    }
    err = WooFNameFromURI(woof, woof_name, sizeof(woof_name));
    if (err < 0) {
        fprintf(stderr, "woof: %s no woof name\n", woof);
        fflush(stderr);
        return (-1);
    }
    woof_name_id = hash(namespace);
    woof_name_id = 1269808015;
#ifdef DEBUG
    printf("namespace: %s, id: %lu\n", namespace, woof_name_id);
    fflush(stdout);
#endif

#ifdef DEBUG
    printf("start dependency discovery\n");
    printf("GLogMarkWooFDownstream name_id:%lu woof_name:%s\n", woof_name_id, woof_name);
    fflush(stdout);
#endif
    GLogMarkWooFDownstream(glog, woof_name_id, woof_name, seq_no);
    // #ifdef DEBUG
    // GLogPrint(stdout, glog);
    // fflush(stdout);
    // #endif

#ifdef DEBUG
    printf("GLogFindAffectedWooF\n");
    fflush(stdout);
#endif
    root = RBInitS();
    casualty = RBInitS();
    progress = RBInitS();
    GLogFindAffectedWooF(
        glog, root, &count_root, casualty, &count_casualty, progress, &count_progress);
#ifdef DEBUG
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf("finished dependency discovery: %f ms\n", elapsedTime);
    fflush(stdout);
#endif
    woof_root = malloc(count_root * sizeof(char*));
    i = 0;
    RB_FORWARD(root, rb) {
        woof_root[i] = malloc(4096 * sizeof(char));
        memset(woof_root[i], 0, sizeof(woof_root[i]));
        sprintf(woof_root[i], "%s", rb->key.key.s);
        free(rb->key.key.s);
        i++;
    }
    woof_casualty = malloc(count_casualty * sizeof(char*));
    i = 0;
    RB_FORWARD(casualty, rb) {
        woof_casualty[i] = malloc(4096 * sizeof(char));
        memset(woof_casualty[i], 0, sizeof(woof_casualty[i]));
        sprintf(woof_casualty[i], "%s", rb->key.key.s);
        free(rb->key.key.s);
        i++;
    }
    woof_progress = malloc(count_progress * sizeof(char*));
    i = 0;
    RB_FORWARD(progress, rb) {
        woof_progress[i] = malloc(4096 * sizeof(char));
        memset(woof_progress[i], 0, sizeof(woof_progress[i]));
        sprintf(woof_progress[i], "%s", rb->key.key.s);
        free(rb->key.key.s);
        i++;
    }
    RBDestroyS(root);
    RBDestroyS(casualty);
    RBDestroyS(progress);

#ifdef DEBUG
    printf("start sending repair requests\n");
    fflush(stdout);
    gettimeofday(&t1, NULL);
#endif

    printf("woof root:\n");
    for (i = 0; i < count_root; i++) {
        wf_name = malloc(4096 * sizeof(char));
        err = WooFFromHval(woof_root[i], &wf_host, wf_name);
        if (err < 0) {
            fprintf(stderr, "cannot parse woof_root %s\n", woof_root[i]);
            fflush(stderr);
            return (-1);
        }
        printf("%lu %s\n", wf_host, wf_name);
        Repair(glog, namespace, wf_host, wf_name);
        free(wf_name);
    }
    printf("woof casualty:\n");
    for (i = 0; i < count_casualty; i++) {
        wf_name = malloc(4096 * sizeof(char));
        err = WooFFromHval(woof_casualty[i], &wf_host, wf_name);
        if (err < 0) {
            fprintf(stderr, "cannot parse woof_casualty %s\n", woof_casualty[i]);
            fflush(stderr);
            return (-1);
        }
        printf("%lu %s\n", wf_host, wf_name);
        Repair(glog, namespace, wf_host, wf_name);
        free(wf_name);
    }
    printf("woof progress:\n");
    for (i = 0; i < count_progress; i++) {
        wf_name = malloc(4096 * sizeof(char));
        err = WooFFromHval(woof_progress[i], &wf_host, wf_name);
        if (err < 0) {
            fprintf(stderr, "cannot parse woof_progress %s\n", woof_progress[i]);
            fflush(stderr);
            return (-1);
        }
        printf("%lu %s\n", wf_host, wf_name);
        RepairRO(glog, namespace, wf_host, wf_name);
        free(wf_name);
    }
#ifdef DEBUG
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    printf("finished sending repair request: %f ms\n", elapsedTime);
    fflush(stdout);
#endif

    Guide(glog, guide_fd);
    fclose(guide_fd);

    // free memory
    for (i = 0; i < num_hosts; i++) {
        free(endpoint[i]);
    }
    free(endpoint);
    RBDestroyI64(seq_no);
    for (i = 0; i < num_hosts; i++) {
        MIOClose(mio[i]);
    }
    free(mio);
    free(log);
    GLogFree(glog);

    return (0);
}

int Repair(GLOG* glog, char* namespace, unsigned long host, char* wf_name) {
    Dlist* holes;
    DlistNode* dn;
    char wf[4096];
    int err;

    holes = DlistInit();
    if (holes == NULL) {
        fprintf(stderr, "Repair: cannot allocate Dlist\n");
        fflush(stderr);
        return (-1);
    }

    GLogFindWooFHoles(glog, host, wf_name, holes);
    // TODO: put host name here, need to make this smarter
    sprintf(wf, "woof:///%s/%s", namespace, wf_name);
    if (host == 7886325280287311374ul) {
        sprintf(wf,
                "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/%s",
                wf_name);
    } else if (host == 797386831364045376ul) {
        sprintf(wf,
                "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/%s",
                wf_name);
    }
#ifdef DEBUG
    printf("Repair: repairing %s(%lu): %d holes\n", wf_name, host, holes->count);
    // DLIST_FORWARD(holes, dn)
    // {
    // 	printf(" %lu", dn->value.i64);
    // }
    // printf("\n");
    fflush(stdout);
#endif
    if (holes->count > 0) {
        if (dryrun == 0) {
            err = WooFRepair(wf_name, holes);
            if (err < 0) {
                fprintf(stderr, "Repair: cannot repair woof %s(%lu)\n", wf_name, host);
                fflush(stderr);
                DlistRemove(holes);
                return (-1);
            }
        }
    }
    DlistRemove(holes);
#endif
    return (0);
}

#ifdef REPAIR
int RepairRO(GLOG* glog, char* namespace, unsigned long host, char* wf_name) {
    RB* asker;
    RB* mapping_count;
    char wf[4096];
    int err;
    RB* rb;
    RB* rbc;
    unsigned long cause_host;
    char cause_woof[4096];

    asker = RBInitS();
    mapping_count = RBInitS();

    GLogFindLatestSeqnoAsker(glog, host, wf_name, asker, mapping_count);
    RB_FORWARD(asker, rb) {
        rbc = RBFindS(mapping_count, rb->key.key.s);
        if (rbc == NULL) {
            fprintf(stderr,
                    "RepairRO: can't find entry %s in mapping_count\n",
                    rb->key.key.s);
            fflush(stderr);
            RBDestroyS(asker);
            RBDestroyS(mapping_count);
            return (-1);
        }
        WooFFromHval(rb->key.key.s, &cause_host, cause_woof);
        // TODO: put host name here, need to make this smarter
        sprintf(wf, "woof:///%s/%s", namespace, wf_name);
        if (host == 7886325280287311374ul) {
            sprintf(wf,
                    "woof://10.1.5.1:51374/home/centos/cspot/apps/regress-pair/cspot/%s",
                    wf_name);
        } else if (host == 797386831364045376ul) {
            sprintf(
                wf,
                "woof://10.1.5.155:55376/home/centos/cspot2/apps/regress-pair/cspot/%s",
                wf_name);
        }
#ifdef DEBUG
        printf(
            "RepairRO: sending mapping from cause_woof %s to %s\n", cause_woof, wf_name);
        fflush(stdout);
#endif
        if (dryrun == 0) {
            err = WooFRepairProgress(wf_name,
                                     cause_host,
                                     cause_woof,
                                     rbc->value.i,
                                     (unsigned long*)rb->value.i64);
            if (err < 0) {
                fprintf(stderr,
                        "RepairRO: cannot repair woof %s/%s(%lu)\n",
                        namespace,
                        wf_name,
                        host);
                fflush(stderr);
                RBDestroyS(asker);
                RBDestroyS(mapping_count);
                return (-1);
            }
        }
    }

    RBDestroyS(asker);
    RBDestroyS(mapping_count);

    return (0);
}

void Guide(GLOG* glog, FILE* fd) {
    EVENT* ev_array;
    LOG* log;
    int err;
    unsigned long curr;
    Hval value;

    // printf("To repair the app, please call WooFPut by the following order:\n");

    log = glog->log;
    ev_array = (EVENT*)(((void*)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size) {
        if (ev_array[curr].type == (TRIGGER | ROOT)) {
            // printf("  host %lu, woof_name %s, seq_no %lu\n", ev_array[curr].host,
            // ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
            fprintf(fd,
                    "%lu,%s,%lu\n",
                    ev_array[curr].host,
                    ev_array[curr].woofc_name,
                    ev_array[curr].woofc_seq_no);
            fflush(fd);
        }
        curr = (curr + 1) % log->size;
    }
    fflush(stdout);
}
#endif