#include "woofc.h"
#include "woofc-priv.h"
#include "log.h"
#include "global.h"
#include "net.h"
#include "debug.h"

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
extern int handler(WOOF* wf, unsigned long seq_no, void* farg);
}

int main(int argc, char** argv, char** envp) {
    cspot::set_active_backend(cspot::get_backend_with_name("zmq"));

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
	ELID l_el_id;
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
	const char* st = "WOOF_HANDLER_NAME";
	int i;
#ifdef TIMING
	double start = strtod(argv[1],NULL);
	double end;
	double end1;
	double endhand;
#endif

	close(2);
	dup2(1,2);
	close(0);

	STOPCLOCK(&end1);
	TIMING_PRINT("SHEP: %lf\n",DURATION(start,end1)*1000);


	if (envp != NULL) {
		i = 0;
		while (envp[i] != NULL) {
			putenv(envp[i]);
			i++;
		}
	}


	DEBUG_LOG("WooFShepherd: called for handler %s\n", st);

	wf_ns = getenv("WOOFC_NAMESPACE");
	if (wf_ns == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOFC_NAMESPACE\n");
		fflush(stdout);
		exit(1);
	}

	strncpy(WooF_namespace, wf_ns, sizeof(WooF_namespace));
	DEBUG_LOG("WooFShepherd: WOOFC_NAMESPACE=%s\n", wf_ns);

	wf_ip = getenv("WOOF_HOST_IP");
	if (wf_ip == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_HOST_IP\n");
		fflush(stdout);
		exit(1);
	}
	strncpy(Host_ip, wf_ip, sizeof(Host_ip));
	DEBUG_LOG("WooFShepherd: WOOF_HOST_IP=%s\n", Host_ip);

	wf_dir = getenv("WOOFC_DIR");
	if (wf_dir == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOFC_DIR\n");
		fflush(stdout);
		exit(1);
	}
	strncpy(WooF_dir, wf_dir, sizeof(WooF_dir));
	DEBUG_LOG("WooFShepherd: WOOFC_DIR=%s\n", wf_dir);

	wnld = getenv("WOOF_NAMELOG_DIR");
	if (wnld == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_NAMELOG_DIR\n");
		fflush(stdout);
		exit(1);
	}
	strncpy(WooF_namelog_dir, wnld, sizeof(WooF_namelog_dir));

	wf_name = getenv("WOOF_SHEPHERD_NAME");
	if (wf_name == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_SHEPHERD_NAME\n");
		fflush(stdout);
		exit(1);
	}
	DEBUG_LOG("WooFShepherd: WOOF_SHEPHERD_NAME=%s\n", wf_name);

	wf_ndx = getenv("WOOF_SHEPHERD_NDX");
	if (wf_ndx == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_SHEPHERD_NDX for %s\n", wf_name);
		fflush(stdout);
		exit(1);
	}
	ndx = strtoul(wf_ndx, (char**)NULL, 10);
	DEBUG_LOG("WooFShepherd: WOOF_SHEPHERD_NDX=%lu\n", ndx);

	wf_seq_no = getenv("WOOF_SHEPHERD_SEQNO");
	if (wf_seq_no == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_SHEPHERD_SEQNO for %s\n", wf_name);
		fflush(stdout);
		exit(1);
	}
	seq_no = strtoul(wf_seq_no, (char**)NULL, 10);
	DEBUG_LOG("WooFShepherd: WOOF_SHEPHERD_SEQ_NO=%lu\n", seq_no);

	namelog_name = getenv("WOOF_NAMELOG_NAME");
	if (namelog_name == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_NAMELOG_NAME\n");
		fflush(stdout);
		exit(1);
	}
	strncpy(Namelog_name, namelog_name, sizeof(Namelog_name));
	DEBUG_LOG("WooFShepherd: WOOF_NAMELOG_NAME=%s\n", namelog_name);

	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if (namelog_seq_no == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_NAMELOG_SEQNO for log %s wf %s\n", namelog_name, wf_name);
		fflush(stdout);
		exit(1);
	}
	my_log_seq_no = strtoul(namelog_seq_no, (char**)NULL, 10);
	DEBUG_LOG("WooFShepherd: WOOF_NAMELOG_SEQNO=%lu\n", my_log_seq_no);

	name_id = getenv("WOOF_NAME_ID");
	if (name_id == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't find WOOF_NAME_ID\n");
		fflush(stdout);
		exit(1);
	}
	Name_id = strtoul(name_id, (char**)NULL, 10);
	DEBUG_LOG("WooFShepherd: WOOF_NAME_ID=%lu\n", Name_id);

	strncpy(full_name, wf_dir, sizeof(full_name));
	if (full_name[strlen(full_name) - 1] != '/') {
		strncat(full_name, "/", sizeof(full_name) - strlen(full_name) - 1);
	}
	strncat(full_name, wf_name, sizeof(full_name) - strlen(full_name) - 1);
	DEBUG_LOG("WooFShepherd: WooF name: %s\n", full_name);


	wf = WooFOpen(wf_name);
	if (wf == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't open %s\n", full_name);
		fflush(stdout);
		exit(1);
	}

    	wfs = wf->shared;
    	buf = (unsigned char*)(((char*)wfs) + sizeof(WOOF_SHARED));
        ptr = buf + (ndx * (wfs->element_size + sizeof(ELID)));
        el_id = (ELID*)(ptr + wfs->element_size);
#ifdef TRACK
	P(&wfs->mutex);
	printf("%s %d PREP seq_no: %lu ndx: %d\n",wfs->filename,el_id->hid, seq_no, ndx);
	fflush(stdout);
	V(&wfs->mutex);
#endif

	sprintf(log_name, "%s/%s", WooF_namelog_dir, namelog_name);
	lmio = MIOReOpen(log_name);
	if (lmio == NULL) {
		fprintf(stdout, "WooFShepherd: couldn't open mio for log %s\n", log_name);
		fflush(stdout);
		exit(1);
	}
	Name_log = (LOG*)MIOAddr(lmio);
	strncpy(Namelog_name, namelog_name, sizeof(Namelog_name));

	farg = (unsigned char*)malloc(wfs->element_size);
	if (farg == NULL) {
	    fprintf(stdout, "WooFShepherd: no space for farg of size %lu\n", wfs->element_size);
	    fflush(stdout);
	    exit(1);
	}
	memcpy(farg, ptr, wfs->element_size);

	/*
	 * possible RACE condition
	 *
	 * if the woof wrapped between the time we appended the woof and
	 * the handler fired, print an error and exit without firing the
	 * handler on the wrong element
	 * 
	 * assume memcpy is thread safe and use a local copy
	 */

	if(el_id->seq_no != seq_no) {
		fprintf(stdout,
	"ERROR: handler %s assigned seq_no %d does not match element seq_no %d -- ",
		wf_name,seq_no,el_id->seq_no);
		fprintf(stdout, "Possible fix is to make the woof bigger\n");
		fflush(stdout);
		exit(0);
	}

#ifdef TRACK
	printf("%s %d FIRE seq_no: %lu ndx: %d pid: %d\n",wfs->filename,el_id->hid, seq_no, ndx, getpid());
	fflush(stdout);
#endif

	WooFSetInit(); // so that a handler that inadevertantly calls WooFInit() does not leak file desc

	STOPCLOCK(&end);
	TIMING_PRINT("HANDAT: %lf ms\n",DURATION(start,end)*1000);
	err = handler(wf, seq_no, (void*)farg);
    	WooFDrop(wf);
    	MIOClose(lmio);
	STOPCLOCK(&endhand);
	TIMING_PRINT("DONE: %lf ms\n",DURATION(start,endhand)*1000);

	exit(0);
	return (0);
}
