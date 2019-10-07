#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

int main(int argc, char **argv)
{
	int err;
	unsigned long seq_no;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char predecessor_woof_name[2048];
	GET_PREDECESSOR_ARG arg;
	char msg[128];
	
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	FILE *f = fopen("log_check_predecessor","w");
	log_set_output(f);
	// log_set_output(stdout);
	WooFInit();
	
	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("check_predecessor", "couldn't get local node's woof name");
		return 0;
	}

	seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq_no))
	{
		log_error("check_predecessor", "couldn't get latest dht_table seq_no");
		return 0;
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq_no);
		log_error("check_predecessor", msg);
		return 0;
	}

	if (dht_tbl.predecessor_addr[0] == 0)
	{
		log_debug("check_predecessor", "predecessor is nil");
		return 0;
	}
	sprintf(msg, "checking predecessor: %s", dht_tbl.predecessor_addr);
	log_debug("check_predecessor", msg);

	// check if predecessor woof is working, do nothing
	sprintf(predecessor_woof_name, "%s/%s", dht_tbl.predecessor_addr, DHT_GET_PREDECESSOR_ARG_WOOF);
	seq_no = WooFPut(predecessor_woof_name, NULL, &arg);
	if (WooFInvalid(seq_no))
	{
		sprintf(msg, "couldn't access predecessor %s", dht_tbl.predecessor_addr);
		log_warn("check_predecessor", msg);

		// predecessor = nil;
		memset(dht_tbl.predecessor_addr, 0, sizeof(dht_tbl.predecessor_addr));
		memset(dht_tbl.predecessor_hash, 0, sizeof(dht_tbl.predecessor_hash));
		seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
		if (WooFInvalid(seq_no))
		{
			log_error("check_predecessor", "couldn't set predecessor to nil");
			return 0;
		}
		log_info("check_predecessor", "set predecessor to nil");
	}
	sprintf(msg, "predecessor %s is working", dht_tbl.predecessor_addr);
	log_debug("check_predecessor", msg);

	return 0;
}
