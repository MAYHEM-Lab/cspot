#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "log.h"
#include "woofc.h"
#include "replica.h"

extern char WooF_namespace[2048];

int replay_event(WOOF *wf, unsigned long seq_no, void *ptr)
{
	SLAVE_PROGRESS *slave_progress = (SLAVE_PROGRESS *)ptr;
	int i;
	int err;
	unsigned long seq;
	EVENT *events;
	EVENT ev;
	char master_namespace[256];
	struct stat sbuf;
	char woof_name[4096];
	char woof_element[4096];
	unsigned long element_size;
	unsigned long history_size;
	char woof_file_in_cspot[4096];
	REPLICA_EL el;
	char master_woof[4096];
	char local_ip[25];
	unsigned int port;

	printf("received %d events\n", slave_progress->number_event);
	fflush(stdout);
	events = slave_progress->event;
	memcpy(master_namespace, slave_progress->master_namespace, sizeof(master_namespace));
	for (i = 0; i < slave_progress->number_event; i++)
	{
		ev = events[i];
		// printf("master[%lu]: type: %d, woof: %s, seqno: %lu\n", ev.seq_no, ev.type, ev.woofc_name, ev.woofc_seq_no);
		sprintf(woof_file_in_cspot, "%s%s", DEFAULT_WOOF_DIR, ev.woofc_name);
		// printf("stat(%s): %d\n", woof_file_in_cspot, stat(woof_file_in_cspot, &sbuf));
		// fflush(stdout);
		if (ev.woofc_name[0] != 0 && stat(woof_file_in_cspot, &sbuf) < 0)
		{
			// get meta
			sprintf(woof_name, "%s/%s", master_namespace, ev.woofc_name);
			element_size = WooFMsgGetElSize(woof_name);
			// history_size = WooFMsgGetHistorySize(ev.woofc_name);
			history_size = 20000;
			if (element_size == 16) // BST_LL_AP
			{
				history_size = 10000;
			}
			else if (element_size == 32) // BST_LL_LINK
			{
				history_size = 20000;
			}
			else if (element_size == 520) // BST_DATA
			{
				history_size = 20000;
			}
			else if (element_size == 528) // LL_DATA
			{
				history_size = 10000;
			}

			err = WooFCreate(ev.woofc_name, element_size, history_size);
			if (err < 0)
			{
				fprintf(stderr, "master[%lu]: can't create woof %s\n", ev.seq_no, ev.woofc_name);
				fflush(stderr);
				exit(1);
			}
			// printf("master[%lu]: created woof %s, element_size: %lu, history_size: %lu\n", ev.seq_no, ev.woofc_name, element_size, history_size);
			// printf("stat(%s): %d\n", woof_file_in_cspot, stat(woof_file_in_cspot, &sbuf));
			// fflush(stdout);
		}
		if (ev.type == LATEST_SEQNO)
		{
			WooFGetLatestSeqno(ev.woofc_name);
		}
		else if (ev.type == READ)
		{
			// printf("master[%lu]: WooFGet(%s, &woof_element, %lu)\n", ev.seq_no, ev.woofc_name, ev.woofc_seq_no);
			// fflush(stdout);
			err = WooFGet(ev.woofc_name, &woof_element, ev.woofc_seq_no);
			if (err < 0){
				fprintf(stderr, "master[%lu]: can't get woof %s[%lu]\n", ev.seq_no, ev.woofc_name, ev.woofc_seq_no);
				fflush(stderr);
				exit(1);
			}
		}
		else if (ev.type == APPEND)
		{
			// printf("master[%lu]: WooFGet(%s, &woof_element, %lu)\n", ev.seq_no, woof_name, ev.woofc_seq_no);
			// fflush(stdout);
			sprintf(woof_name, "%s/%s", master_namespace, ev.woofc_name);
			err = WooFGet(woof_name, &woof_element, ev.woofc_seq_no);
			if (err < 0){
				fprintf(stderr, "master[%lu]: can't get woof %s[%lu]\n", ev.seq_no, woof_name, ev.woofc_seq_no);
				fflush(stderr);
				exit(1);
			}
			// printf("master[%lu]: WooFPut(%s, NULL, &woof_element)\n", ev.seq_no, ev.woofc_name);
			// fflush(stdout);
			seq = WooFPut(ev.woofc_name, NULL, &woof_element);
			if (WooFInvalid(seq))
			{
				fprintf(stderr, "master[%lu]: can't put to woof %s\n", ev.seq_no, ev.woofc_name);
				fflush(stderr);
				exit(1);
			}
			// printf("master[%lu]: WooFPut => %lu\n", ev.seq_no, seq);
			// fflush(stdout);
		}
	}

	if (slave_progress->number_event == 0)
	{
		// usleep(500000);
		sleep(1);
	}
	el.log_seqno = slave_progress->log_seqno;
	port = WooFPortHash(WooF_namespace);
	err = WooFLocalIP(local_ip, sizeof(local_ip));
	if (err < 0)
	{
		fprintf(stderr, "couldn't get local IP\n");
		fflush(stderr);
		exit(1);
	}
	sprintf(el.events_woof, "woof://%s:%u%s/events_woof_for_replica", local_ip, port, WooF_namespace);
	sprintf(master_woof, "%s/slave_progress_for_replica", master_namespace);
	seq = WooFPut(master_woof, "replicate_event", (void *)&el);
	if (WooFInvalid(seq))
	{
		fprintf(stderr, "failed to put to %s\n", master_woof);
		fflush(stderr);
		exit(1);
	}
	printf("ask master %s for more events\n", master_woof);
	fflush(stdout);

	return 1;
}
