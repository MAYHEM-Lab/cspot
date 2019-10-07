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

void get_finger_id(unsigned char *dst, const unsigned char *n, int i)
{
	int j;
	int carry = 0;
	// dst += 2^i
	for (j = SHA_DIGEST_LENGTH - 1; j >= 0; j--)
	{
		dst[j] = carry;
		if (SHA_DIGEST_LENGTH - 1 - (i / 8) == j)
		{
			dst[j] += (1 << (i % 8));
		}
		if (n[j] >= (256 - dst[j]))
		{
			carry = 1;
		}
		else
		{
			carry = 0;
		}
		dst[j] += n[j];
	}
#ifdef DEBUG
	printf("get_finger_id: dst=(n + 2 ^ %d)\n", i);
	printf("get_finger_id:   n= ");
	for (j = 0; j < SHA_DIGEST_LENGTH; j++)
	{
		printf("%d ", n[j]);
	}
	printf("\nget_finger_id: dst= ");
	for (j = 0; j < SHA_DIGEST_LENGTH; j++)
	{
		printf("%d ", dst[j]);
	}
	printf("\n");
#endif
}

int main(int argc, char **argv)
{
	int i;
	int j;
	int err;
	char woof_name[2048];
	unsigned long seq_no;
	FIND_SUCESSOR_ARG arg;
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char msg[128];

	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	FILE *f = fopen("log_fix_fingers","w");
	log_set_output(f);
	// log_set_output(stdout);
	WooFInit();

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("fix_fingers", "couldn't get local node's woof name");
		exit(1);
	}
	
	i = atoi(argv[1]);

	// compute the node hash with SHA1
	SHA1(woof_name, strlen(woof_name), node_hash);
	
	sprintf(msg, "fixing finger[%d]", i);
	log_debug("fix_fingers", msg);

	// finger[i] = find_successor(n + 2^i)
	// finger_hash = n + 2^i
	get_finger_id(arg.id_hash, node_hash, i);
	arg.finger_index = i;
	sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCESSOR_RESULT_WOOF);
	sprintf(arg.callback_handler, "fix_fingers_callback", sizeof(arg.callback_handler));

	seq_no = WooFPut(DHT_FIND_SUCESSOR_ARG_WOOF, "find_successor", &arg);
	if (WooFInvalid(seq_no))
	{
		sprintf(msg, "couldn't call find_successor on woof %s", DHT_FIND_SUCESSOR_ARG_WOOF);
		log_error("fix_fingers", msg);
		return 0;
	}
	
	return 0;
}
