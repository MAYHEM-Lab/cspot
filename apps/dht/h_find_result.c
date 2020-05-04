#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "dht.h"

int h_find_result(WOOF *wf, unsigned long seq_no, void *ptr) {
	FIND_SUCESSOR_RESULT *result = (FIND_SUCESSOR_RESULT *)ptr;

	log_set_tag("find_result");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	log_info("find_result_seq_no: %lu, request_seq_no: %lu, hops: %d", seq_no, result->request_seq_no, result->hops);
	log_info("finger_index: %d", result->finger_index);
	char msg[256];
	sprintf(msg, "node_hash: ");
	print_node_hash(msg + strlen(msg), result->node_hash);
	log_info(msg);
	log_info("node_addr: %s", result->node_addr);

	return 1;
}
