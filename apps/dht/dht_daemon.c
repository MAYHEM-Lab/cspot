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

int stablize_freq = 1000;		 // 1 secs
int chk_predecessor_freq = 1000; // 1 secs
int fix_fingers_freq = 100;		 // 100 msec (for each finger)

static int running = 1;
static int thread_finished = 0;
pthread_t stablize_thread;
pthread_t check_predecessor_thread;
pthread_t fix_fingers_thread;

void *stablize(void *ptr);
void *check_predecessor(void *ptr);
void *fix_fingers(void *ptr);
void get_finger_id(unsigned char *dst, const unsigned char *n, int i);

void signal_handler(int signum)
{
	log_info("dht_daemon", "SIGINT caught");
	running = 0;
	pthread_join(stablize_thread, NULL);
	pthread_join(check_predecessor_thread, NULL);
	pthread_join(fix_fingers_thread, NULL);
}

int main(int argc, char **argv)
{
	int i;
	int err;
	unsigned long seq_no;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char successor_woof_name[2048];
	GET_PREDECESSOR_ARG arg;
	char msg[128];

	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	// FILE *f = fopen("log_daemon","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	sprintf(msg, "stablize every %d ms", stablize_freq);
	log_debug("dht_daemon", msg);
	sprintf(msg, "check_predecessor every %d ms", chk_predecessor_freq);
	log_debug("dht_daemon", msg);
	sprintf(msg, "fix_fingers every %d ms", fix_fingers_freq);
	log_debug("dht_daemon", msg);
	signal(SIGINT, signal_handler);

	pthread_create(&stablize_thread, NULL, stablize, (void *)NULL);
	// usleep(stablize_freq * 1000 / 2);
	pthread_create(&check_predecessor_thread, NULL, check_predecessor, (void *)NULL);
	pthread_create(&fix_fingers_thread, NULL, fix_fingers, (void *)NULL);
	while (running || thread_finished < 3)
	{
		sleep(1);
	}

	return 0;
}

void *stablize(void *ptr)
{
	int err;
	unsigned long seq_no;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char successor_woof_name[2048];
	char msg[128];

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("stablize", "couldn't get local node's woof name");
		return;
	}

	while (running)
	{
		GET_PREDECESSOR_ARG arg;
		NOTIFY_ARG notify_arg;
		usleep(stablize_freq * 1000);

		seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
		if (WooFInvalid(seq_no))
		{
			log_error("stablize", "couldn't get latest dht_table seq_no");
			continue;
		}
		err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no);
		if (err < 0)
		{
			sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq_no);
			log_error("stablize", msg);
			continue;
		}

		if (memcmp(dht_tbl.finger_hash[0], dht_tbl.node_hash, SHA_DIGEST_LENGTH) == 0)
		{
			sprintf(msg, "current successor is its self");
			log_debug("stablize", msg);

			// successor = predecessor;
			if ((dht_tbl.predecessor_hash[0] != 0)
				&& (memcmp(dht_tbl.finger_hash[0], dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH) != 0))
			{
				memcpy(dht_tbl.finger_hash[0], dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH);
				strncpy(dht_tbl.finger_addr[0], dht_tbl.predecessor_addr, sizeof(dht_tbl.finger_addr[0]));
				seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
				if (WooFInvalid(seq_no))
				{
					log_error("stablize", "couldn't update successor");
					continue;
				}

				sprintf(msg, "updated successor to %s", dht_tbl.finger_addr[0]);
				log_info("stablize", msg);
			}

			// successor.notify(n);
			memcpy(notify_arg.node_hash, dht_tbl.node_hash, sizeof(notify_arg.node_hash));
			strncpy(notify_arg.node_addr, dht_tbl.node_addr, sizeof(notify_arg.node_addr));

			seq_no = WooFPut(DHT_NOTIFY_ARG_WOOF, "notify", &notify_arg);
			if (WooFInvalid(seq_no))
			{
				sprintf(msg, "couldn't call notify on self %s", dht_tbl.node_addr);
				log_error("stablize", msg);
				continue;
			}

			sprintf(msg, "called notify on self %s", dht_tbl.finger_addr[0]);
			log_debug("stablize", msg);
		}
		else
		{
			sprintf(msg, "current successor_addr: %s", dht_tbl.finger_addr[0]);
			// log_debug("stablize", msg);
			log_info("stablize", msg);

			sprintf(successor_woof_name, "%s/%s", dht_tbl.finger_addr[0], DHT_GET_PREDECESSOR_ARG_WOOF);
			sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCESSOR_RESULT_WOOF);
			sprintf(arg.callback_handler, "stablize_callback", sizeof(arg.callback_handler));

			// x = successor.predecessor
			seq_no = WooFPut(successor_woof_name, "get_predecessor", &arg);
			if (WooFInvalid(seq_no))
			{
				sprintf(msg, "couldn't get put to woof %s to get_predecessor", successor_woof_name);
				log_error("stablize", msg);
				continue;
			}
			sprintf(msg, "asked to get_predecessor from %s", successor_woof_name);
			log_debug("stablize", msg);
		}
	}
	thread_finished++;
}

void *check_predecessor(void *ptr)
{
	int err;
	unsigned long seq_no;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char predecessor_woof_name[2048];
	char msg[128];
	int i;

	while (running)
	{
		GET_PREDECESSOR_ARG arg;
		usleep(chk_predecessor_freq * 1000);
		
		err = node_woof_name(woof_name);
		if (err < 0)
		{
			log_error("check_predecessor", "couldn't get local node's woof name");
			continue;
		}

		seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
		if (WooFInvalid(seq_no))
		{
			log_error("check_predecessor", "couldn't get latest dht_table seq_no");
			continue;
		}
		sprintf(msg, "seq: %lu\n", seq_no);
		log_debug("finger_addr", msg);
		err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no);
		if (err < 0)
		{
			sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq_no);
			log_error("check_predecessor", msg);
			continue;
		}

		sprintf(msg, "current predecessor_addr: %s", dht_tbl.predecessor_addr);
		// log_debug("check_predecessor", msg);
		log_info("check_predecessor", msg);
		if (dht_tbl.predecessor_addr[0] == 0)
		{
			log_debug("check_predecessor", "predecessor is nil");
			continue;
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
			memset(dht_tbl.predecessor_addr, 0, sizeof(char) * 256);
			memset(dht_tbl.predecessor_hash, 0, sizeof(unsigned char) * SHA_DIGEST_LENGTH);
			seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
			if (WooFInvalid(seq_no))
			{
				log_error("check_predecessor", "couldn't set predecessor to nil");
				continue;
			}
			log_debug("check_predecessor", "set predecessor to nil");
		}
		sprintf(msg, "predecessor %s is working", dht_tbl.predecessor_addr);
		log_debug("check_predecessor", msg);

		// print fingers
		sprintf(msg, "finger_addr ");
		for (i = 1; i <= SHA_DIGEST_LENGTH * 8; i++) {
			sprintf(msg + strlen(msg), "%c ", dht_tbl.finger_addr[i][strlen(dht_tbl.finger_addr[i]) - 16]);
		}
		log_debug("finger_addr", msg);
	}
	thread_finished++;
}

void *fix_fingers(void *ptr)
{
	int i;
	int j;
	int err;
	char woof_name[2048];
	unsigned long seq_no;
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char msg[128];
	int finger_id;

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("fix_fingers", "couldn't get local node's woof name");
		exit(1);
	}

	// compute the node hash with SHA1
	SHA1(woof_name, strlen(woof_name), node_hash);
	i = 0;
	while (running)
	{
		FIND_SUCESSOR_ARG arg;
		finger_id = i + 1;

		// finger[i] = find_successor(n + 2^(i-1))
		// finger_hash = n + 2^(i-1)
		get_finger_id(arg.id_hash, node_hash, finger_id);
		sprintf(msg, "fixing finger[%d](", finger_id);
		print_node_hash(msg + strlen(msg), arg.id_hash);
		log_debug("fix_fingers", msg);
		
		arg.finger_index = finger_id;
		sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCESSOR_RESULT_WOOF);
		sprintf(arg.callback_handler, "fix_fingers_callback", sizeof(arg.callback_handler));

		seq_no = WooFPut(DHT_FIND_SUCESSOR_ARG_WOOF, "find_successor", &arg);
		if (WooFInvalid(seq_no))
		{
			sprintf(msg, "couldn't call find_successor on woof %s", DHT_FIND_SUCESSOR_ARG_WOOF);
			log_error("fix_fingers", msg);
			usleep(fix_fingers_freq * 1000);
			continue;
		}
		i = (i + 1) % (SHA_DIGEST_LENGTH * 8);
		usleep(fix_fingers_freq * 1000);
	}
	thread_finished++;
}

void get_finger_id(unsigned char *dst, const unsigned char *n, int i)
{
	int j;
	unsigned char carry = 0;
	// dst = n + 2^(i-1)
	int shift = i - 1;
	for (j = SHA_DIGEST_LENGTH - 1; j >= 0; j--)
	{
		dst[j] = carry;
		if (SHA_DIGEST_LENGTH - 1 - (shift / 8) == j)
		{
			dst[j] += (unsigned char)(1 << (shift % 8));
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
	printf("get_finger_id: dst=(n + 2 ^ (%d - 1))\n", i);
	char msg[256];
	print_node_hash(msg, n);
	printf("get_finger_id:   n= %s\n", msg);
	print_node_hash(msg, dst);
	printf("get_finger_id: dst= %s\n", msg);
#endif
}