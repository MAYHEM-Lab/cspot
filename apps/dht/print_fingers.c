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
	int i;
	int err;
	unsigned long seq_no;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char hash[256];
	unsigned char finger_node[161][16];
	
	memset(finger_node, 0, sizeof(unsigned char) * 16 * 161);
	WooFInit();
	
	err = node_woof_name(woof_name);
	if (err < 0)
	{
		fprintf(stderr, "couldn't get local node's woof name");
		fflush(stderr);
		return 0;
	}

	seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq_no))
	{
		fprintf(stderr, "couldn't get latest dht_table seq_no");
		fflush(stderr);
		return 0;
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no);
	if (err < 0)
	{
		fprintf(stderr, "couldn't get latest dht_table with seq_no %lu", seq_no);
		fflush(stderr);
		return 0;
	}

	for (i = 1; i <= SHA_DIGEST_LENGTH * 8; i++) {
		// print_node_hash(hash, dht_tbl.finger_hash[i]);
		// printf("%d: %s(%s) %c\n", i, hash, dht_tbl.finger_addr[i], dht_tbl.finger_addr[i][strlen(dht_tbl.finger_addr[i]) - 16]);
		// fflush(stdout);
		if (strlen(dht_tbl.finger_addr[i]) == 0) {
			sprintf(finger_node[i], "_");
		} else {
			sprintf(finger_node[i], "%c", dht_tbl.finger_addr[i][strlen(dht_tbl.finger_addr[i]) - 16]);
		}
	}

	printf("seq: %lu\n", seq_no);
	for (i = 1; i <= SHA_DIGEST_LENGTH * 8; i++) {
		printf("%s ", finger_node[i]);
		fflush(stdout);
	}
	return 0;
}
