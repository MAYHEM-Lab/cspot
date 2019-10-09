#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "dht.h"

int find_result(WOOF *wf, unsigned long seq_no, void *ptr)
{
	FIND_SUCESSOR_RESULT *result = (FIND_SUCESSOR_RESULT *)ptr;
	char msg[256];

	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	sprintf(msg, "find_result_seq_no: %lu, request_seq_no: %lu, hops: %d", seq_no, result->request_seq_no, result->hops);
	log_info("find_result", msg);
	sprintf(msg, "finger_index: %d", result->finger_index);
	log_info("find_result", msg);
	sprintf(msg, "node_hash: ");
	print_node_hash(msg + strlen(msg), result->node_hash);
	log_info("find_result", msg);
	sprintf(msg, "node_addr: %s", result->node_addr);
	log_info("find_result", msg);
	fflush(stdout);

	return 1;
}
