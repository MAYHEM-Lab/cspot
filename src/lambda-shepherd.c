#include "woofc.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


LOG* Name_log;
unsigned long Name_id;
char WooF_namespace[2048];
char WooF_dir[2048];
char Host_ip[25];
char WooF_namelog_dir[2048];
char Namelog_name[2048];


int main(int argc, char* key, void* data) {
    char* wnld;
    char* wf_ns;
    char* wf_dir;
    char* wf_name;
    char* wf_size;
    char* wf_ndx;
    char* wf_seq_no;
    char* wf_ip;
    char* namelog_name;
    char* namelog_size_str;
    char* namelog_seq_no;
    char* name_id;
    MIO* mio;
    MIO* lmio;
    char full_name[2048];
    char log_name[4096];

    WOOF* wf;
    WOOF_SHARED* wfs;
    ELID* el_id;
    unsigned char* buf;
    unsigned char* ptr;
    unsigned char* farg;
    unsigned long ndx;
    unsigned char* el_buf;
    unsigned long seq_no;
    unsigned long next;
    unsigned long my_log_seq_no; /* needed for logging cause */
    unsigned long namelog_size;
    int err;
    char* st = "WOOF_HANDLER_NAME";

    /*
     * close stdin to make docker happy
     */
    fclose(stdin);
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: called for handler %s\n", st);
    fflush(stdout);
#endif

    wf_ns = getenv("WOOFC_NAMESPACE");
    if (wf_ns == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOFC_NAMESPACE\n");
        fflush(stderr);
        exit(1);
    }

    strncpy(WooF_namespace, wf_ns, sizeof(WooF_namespace));

#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOFC_NAMESPACE=%s\n", wf_ns);
    fflush(stdout);
#endif

    wf_ip = getenv("WOOF_HOST_IP");
    if (wf_ip == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_HOST_IP\n");
        fflush(stderr);
        exit(1);
    }
    strncpy(Host_ip, wf_ip, sizeof(Host_ip));
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_HOST_IP=%s\n", Host_ip);
    fflush(stdout);
#endif

    wf_dir = getenv("WOOFC_DIR");
    if (wf_dir == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOFC_DIR\n");
        fflush(stderr);
        exit(1);
    }

    strncpy(WooF_dir, wf_dir, sizeof(WooF_dir));

#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOFC_DIR=%s\n", wf_dir);
    fflush(stdout);
#endif

    wnld = getenv("WOOF_NAMELOG_DIR");
    if (wnld == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_NAMELOG_DIR\n");
        fflush(stderr);
        exit(1);
    }
    strncpy(WooF_namelog_dir, wnld, sizeof(WooF_namelog_dir));

    wf_name = getenv("WOOF_SHEPHERD_NAME");
    if (wf_name == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_SHEPHERD_NAME\n");
        fflush(stderr);
        exit(1);
    }

#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_SHEPHERD_NAME=%s\n", wf_name);
    fflush(stdout);
#endif

    wf_ndx = getenv("WOOF_SHEPHERD_NDX");
    if (wf_ndx == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_SHEPHERD_NDX for %s\n", wf_name);
        fflush(stderr);
        exit(1);
    }
    ndx = (unsigned long)atol(wf_ndx);

#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_SHEPHERD_NDX=%lu\n", ndx);
    fflush(stdout);
#endif

    wf_seq_no = getenv("WOOF_SHEPHERD_SEQNO");
    if (wf_seq_no == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_SHEPHERD_SEQNO for %s\n", wf_name);
        fflush(stderr);
        exit(1);
    }
    seq_no = (unsigned long)atol(wf_seq_no);
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_SHEPHERD_SEQ_NO=%lu\n", seq_no);
    fflush(stdout);
#endif

    namelog_name = getenv("WOOF_NAMELOG_NAME");
    if (namelog_name == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_NAMELOG_NAME\n");
        fflush(stderr);
        exit(1);
    }
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_NAMELOG_NAME=%s\n", namelog_name);
    fflush(stdout);
#endif

    strncpy(Namelog_name, namelog_name, sizeof(Namelog_name));
    namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
    if (namelog_seq_no == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_NAMELOG_SEQNO for log %s wf %s\n", namelog_name, wf_name);
        fflush(stderr);
        exit(1);
    }
    my_log_seq_no = (unsigned long)atol(namelog_seq_no);
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_NAMELOG_SEQNO=%lu\n", my_log_seq_no);
    fflush(stdout);
#endif

    name_id = getenv("WOOF_NAME_ID");
    if (name_id == NULL) {
        fprintf(stderr, "WooFShepherd: couldn't find WOOF_NAME_ID\n");
        fflush(stderr);
        exit(1);
    }

    Name_id = (unsigned long)atol(name_id);
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WOOF_NAME_ID=%lu\n", Name_id);
    fflush(stdout);
#endif

    strncpy(full_name, wf_dir, sizeof(full_name));
    if (full_name[strlen(full_name) - 1] != '/') {
        strncat(full_name, "/", sizeof(full_name) - strlen(full_name));
    }
    strncat(full_name, wf_name, sizeof(full_name) - strlen(full_name));
#ifdef DEBUG
    fprintf(stdout, "WooFShepherd: WooF name: %s\n", full_name);
    fflush(stdout);
#endif

    // Fill with DynamoDB information
    // char *ddb_key = getenv("DDB_KEY");
    wf = (WOOF*)malloc(sizeof(WOOF));
    wf->shared = (WOOF_SHARED*)malloc(sizeof(WOOF_SHARED));
    strcpy(wf->shared->filename, key);
    // DynamoDB
    // ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
    // farg = (unsigned char *)malloc(wfs->element_size);
    // memcpy(farg,ptr,wfs->element_size);

    /*
     * now that we have the argument copied, free the slot in the woof
     */

    err = hw(wf, seq_no, (void*)data);
    // free(farg);

    return (0);
}
