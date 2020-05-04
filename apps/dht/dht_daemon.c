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

void signal_handler(int signum) {
	log_info("SIGINT caught");
	running = 0;
	pthread_join(stablize_thread, NULL);
	pthread_join(check_predecessor_thread, NULL);
	pthread_join(fix_fingers_thread, NULL);
}

int main(int argc, char **argv) {
	log_set_tag("dht_daemon");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	// FILE *f = fopen("log_daemon","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	log_info("stablize every %d ms", stablize_freq);
	log_info("check_predecessor every %d ms", chk_predecessor_freq);
	log_info("fix_fingers every %d ms", fix_fingers_freq);
	signal(SIGINT, signal_handler);

	pthread_create(&stablize_thread, NULL, stablize, (void *)NULL);
	pthread_create(&check_predecessor_thread, NULL, check_predecessor, (void *)NULL);
	pthread_create(&fix_fingers_thread, NULL, fix_fingers, (void *)NULL);
	while (running || thread_finished < 3) {
		sleep(1);
	}

	return 0;
}

void *stablize(void *ptr) {
	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("stablize: couldn't get local node's woof name");
		return;
	}

	while (running) {
		usleep(stablize_freq * 1000);

		unsigned long seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
		if (WooFInvalid(seq_no)) {
			log_error("stablize: couldn't get latest dht_table seq_no");
			continue;
		}
		DHT_TABLE_EL dht_tbl;
		if (WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no) < 0) {
			log_error("stablize: couldn't get latest dht_table with seq_no %lu", seq_no);
			continue;
		}

		if (memcmp(dht_tbl.successor_hash[0], dht_tbl.node_hash, SHA_DIGEST_LENGTH) == 0) {
			log_debug("stablize: current successor is its self");

			// successor = predecessor;
			if ((dht_tbl.predecessor_hash[0] != 0)
				&& (memcmp(dht_tbl.successor_hash[0], dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH) != 0)) {
				memcpy(dht_tbl.successor_hash[0], dht_tbl.predecessor_hash, sizeof(dht_tbl.successor_hash[0]));
				memcpy(dht_tbl.successor_addr[0], dht_tbl.predecessor_addr, sizeof(dht_tbl.successor_addr[0]));
				seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
				if (WooFInvalid(seq_no)) {
					log_error("stablize: couldn't update successor");
					continue;
				}
				log_info("stablize: updated successor to %s", dht_tbl.successor_addr[0]);
			}

			// successor.notify(n);
			NOTIFY_ARG notify_arg;
			memcpy(notify_arg.node_hash, dht_tbl.node_hash, sizeof(notify_arg.node_hash));
			memcpy(notify_arg.node_addr, dht_tbl.node_addr, sizeof(notify_arg.node_addr));

			seq_no = WooFPut(DHT_NOTIFY_ARG_WOOF, "h_notify", &notify_arg);
			if (WooFInvalid(seq_no)) {
				log_error("stablize: couldn't call notify on self %s", dht_tbl.node_addr);
				continue;
			}
			log_debug("stablize: calling notify on self %s", dht_tbl.successor_addr[0]);
		} else if (dht_tbl.successor_addr[0][0] == 0) {
			log_info("stablize: no successor, set it back to self");
			memcpy(dht_tbl.successor_addr[0], dht_tbl.node_addr, sizeof(dht_tbl.successor_addr[0]));
			memcpy(dht_tbl.successor_hash[0], dht_tbl.node_hash, sizeof(dht_tbl.successor_hash[0]));
			seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
			if (WooFInvalid(seq_no)) {
				log_error("stablize: couldn't set successor back to self");
				continue;
			}
			log_info("stablize: successor set to self: %s", dht_tbl.successor_addr[0]);
		} else {
			log_debug("stablize: current successor_addr: %s", dht_tbl.successor_addr[0]);

			char successor_woof_name[DHT_NAME_LENGTH];
			sprintf(successor_woof_name, "%s/%s", dht_tbl.successor_addr[0], DHT_GET_PREDECESSOR_ARG_WOOF);
			GET_PREDECESSOR_ARG arg;
			sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCCESSOR_RESULT_WOOF);
			sprintf(arg.callback_handler, "h_stablize_callback");

			// x = successor.predecessor
			seq_no = WooFPut(successor_woof_name, "h_get_predecessor", &arg);
			if (WooFInvalid(seq_no)) {
				log_warn("stablize: couldn't put to woof %s to get_predecessor", successor_woof_name);

				// couldn't contact successor, use the next one
				shift_successor_list(dht_tbl.successor_addr, dht_tbl.successor_hash);
				seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
				if (WooFInvalid(seq_no)) {
					log_error("stablize: couldn't shift successor");
					continue;
				}
				log_info("stablize: successor shifted. new: %s", dht_tbl.successor_addr[0]);
				continue;
			}
			log_debug("stablize: asked to get_predecessor from %s", successor_woof_name);
		}
	}
	thread_finished += 1;
}

void *check_predecessor(void *ptr) {
	while (running)
	{
		usleep(chk_predecessor_freq * 1000);
		
		char woof_name[DHT_NAME_LENGTH];
		if (node_woof_name(woof_name) < 0) {
			log_error("check_predecessor: couldn't get local node's woof name");
			continue;
		}

		unsigned long seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
		if (WooFInvalid(seq_no)) {
			log_error("check_predecessor: couldn't get latest dht_table seq_no");
			continue;
		}
		DHT_TABLE_EL dht_tbl;
		if (WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no) < 0) {
			log_error("check_predecessor: couldn't get latest dht_table with seq_no %lu", seq_no);
			continue;
		}

		// log_debug("check_predecessor", msg);
		log_debug("check_predecessor: current predecessor_addr: %s", dht_tbl.predecessor_addr);
		if (dht_tbl.predecessor_addr[0] == 0) {
			log_debug("check_predecessor: predecessor is nil");
			continue;
		}
		log_debug("check_predecessor: checking predecessor: %s", dht_tbl.predecessor_addr);

		// check if predecessor woof is working, do nothing
		char predecessor_woof_name[DHT_NAME_LENGTH];
		sprintf(predecessor_woof_name, "%s/%s", dht_tbl.predecessor_addr, DHT_GET_PREDECESSOR_ARG_WOOF);
		GET_PREDECESSOR_ARG arg;
		seq_no = WooFPut(predecessor_woof_name, NULL, &arg);
		if (WooFInvalid(seq_no)) {
			log_warn("check_predecessor: couldn't access predecessor %s", dht_tbl.predecessor_addr);

			// predecessor = nil;
			memset(dht_tbl.predecessor_addr, 0, sizeof(dht_tbl.predecessor_addr));
			memset(dht_tbl.predecessor_hash, 0, sizeof(dht_tbl.predecessor_hash));
			seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
			if (WooFInvalid(seq_no)) {
				log_error("check_predecessor: couldn't set predecessor to nil");
				continue;
			}
			log_warn("check_predecessor: set predecessor to nil");
		}
		log_debug("check_predecessor: predecessor %s is working", dht_tbl.predecessor_addr);
	}
	thread_finished += 1;
}

void *fix_fingers(void *ptr) {
	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("fix_fingers: couldn't get local node's woof name");
		exit(1);
	}

	// compute the node hash with SHA1
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), node_hash);
	int i = 0;
	while (running) {
		int finger_id = i + 1;

		// finger[i] = find_successor(n + 2^(i-1))
		// finger_hash = n + 2^(i-1)
		FIND_SUCESSOR_ARG arg;
		get_finger_id(arg.id_hash, node_hash, finger_id);
		// sprintf(msg, "fixing finger[%d](", finger_id);
		// print_node_hash(msg + strlen(msg), arg.id_hash);
		// log_debug("fix_fingers", msg);
		
		arg.finger_index = finger_id;
		sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCCESSOR_RESULT_WOOF);
		sprintf(arg.callback_handler, "h_fix_fingers_callback");

		unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_ARG_WOOF, "h_find_successor", &arg);
		if (WooFInvalid(seq_no)) {
			log_error("fix_fingers: couldn't call find_successor on woof %s", DHT_FIND_SUCCESSOR_ARG_WOOF);
			usleep(fix_fingers_freq * 1000);
			continue;
		}
		i = (i + 1) % (SHA_DIGEST_LENGTH * 8);
		usleep(fix_fingers_freq * 1000);
	}
	thread_finished += 1;
}

void get_finger_id(unsigned char *dst, const unsigned char *n, int i) {
	unsigned char carry = 0;
	// dst = n + 2^(i-1)
	int shift = i - 1;
	int j;
	for (j = SHA_DIGEST_LENGTH - 1; j >= 0; j--) {
		dst[j] = carry;
		if (SHA_DIGEST_LENGTH - 1 - (shift / 8) == j) {
			dst[j] += (unsigned char)(1 << (shift % 8));
		}
		if (n[j] >= (256 - dst[j])) {
			carry = 1;
		} else {
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