#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#include "dht.h"

void dht_init(unsigned char *node_hash, char *node_addr, DHT_TABLE_EL *el)
{
	memcpy(el->node_hash, node_hash, sizeof(el->node_hash));
	strncpy(el->node_addr, node_addr, sizeof(el->node_addr));
	memset(el->finger_addr, 0, sizeof(char) * (SHA_DIGEST_LENGTH * 8 + 1) * 256);
	memset(el->finger_hash, 0, sizeof(unsigned char) * (SHA_DIGEST_LENGTH * 8 + 1) * SHA_DIGEST_LENGTH);
	memset(el->successor_addr, 0, sizeof(char) * DHT_SUCCESSOR_LIST_R * 256);
	memset(el->successor_hash, 0, sizeof(unsigned char) * DHT_SUCCESSOR_LIST_R * SHA_DIGEST_LENGTH);
	memcpy(el->successor_hash[0], node_hash, sizeof(el->successor_hash[0]));
	strncpy(el->successor_addr[0], node_addr, sizeof(el->successor_addr[0]));
	memset(el->predecessor_addr, 0, sizeof(char) * 256);
	memset(el->predecessor_hash, 0, sizeof(unsigned char) * SHA_DIGEST_LENGTH);
}

void find_init(char *node_addr, char *callback, char *topic, FIND_SUCESSOR_ARG *arg)
{
	arg->request_seq_no = 0;
	arg->finger_index = 0;
	arg->hops = 0;
	SHA1(topic, strlen(topic), arg->id_hash);
	sprintf(arg->callback_woof, "%s/%s", node_addr, DHT_FIND_SUCCESSOR_RESULT_WOOF);
	strncpy(arg->callback_handler, callback, sizeof(arg->callback_handler));
}

int node_woof_name(char *node_woof)
{
	int err;
	char local_ip[25];
	char namespace[256];
	char *str;

	str = getenv("WOOFC_NAMESPACE");
	if (str == NULL)
	{
		getcwd(namespace, sizeof(namespace));
	}
	else
	{
		strncpy(namespace, str, sizeof(namespace));
	}
	err = WooFLocalIP(local_ip, sizeof(local_ip));
	if (err < 0)
	{
		fprintf(stderr, "no local IP\n");
		fflush(stderr);
		return -1;
	}
	sprintf(node_woof, "woof://%s%s", local_ip, namespace);
	return 0;
}

void print_node_hash(char *dst, const unsigned char *id_hash)
{
	int i;
	sprintf(dst, "");
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(dst, "%s%02X", dst, id_hash[i]);
	}
}

int in_range(unsigned char *n, unsigned char *lower, unsigned char *upper)
{
	if (memcmp(lower, upper, SHA_DIGEST_LENGTH) < 0)
	{
		if (memcmp(lower, n, SHA_DIGEST_LENGTH) < 0 && memcmp(n, upper, SHA_DIGEST_LENGTH) < 0)
		{
			return 1;
		}
		return 0;
	}
	if (memcmp(lower, n, SHA_DIGEST_LENGTH) < 0 || memcmp(n, upper, SHA_DIGEST_LENGTH) < 0)
	{
		return 1;
	}
	return 0;
}

void log_set_level(int level)
{
	dht_log_level = level;
}

void log_set_output(FILE *file)
{
	dht_log_output = file;
	// log_set_level(LOG_DEBUG);
}

void log_debug(const char *tag, const char *message)
{
	if (dht_log_level > LOG_DEBUG)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(dht_log_output, "\033[0;32m");
	fprintf(dht_log_output, "DEBUG| %.19s [%s]: %s\n", ctime(&now), tag, message);
	fprintf(dht_log_output, "\033[0m");
}

void log_info(const char *tag, const char *message)
{
	if (dht_log_level > LOG_INFO)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(dht_log_output, "\033[0;34m");
	fprintf(dht_log_output, "INFO | %.19s [%s]: %s\n", ctime(&now), tag, message);
	fprintf(dht_log_output, "\033[0m");
}

void log_warn(const char *tag, const char *message)
{
	if (dht_log_level > LOG_WARN)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(dht_log_output, "\033[0;33m");
	fprintf(dht_log_output, "WARN | %.19s [%s]: %s\n", ctime(&now), tag, message);
	fprintf(dht_log_output, "\033[0m");
}

void log_error(const char *tag, const char *message)
{
	if (dht_log_level > LOG_ERROR)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(dht_log_output, "\033[0;31m");
	fprintf(dht_log_output, "ERROR| %.19s [%s]: %s\n", ctime(&now), tag, message);
	fprintf(dht_log_output, "\033[0m");
}

void shift_successor_list(char successor_addr[DHT_SUCCESSOR_LIST_R][256], unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH])
{
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; i++)
	{
		memcpy(successor_addr[i], successor_addr[i + 1], 256);
		memcpy(successor_hash[i], successor_hash[i + 1], SHA_DIGEST_LENGTH);
	}
	memset(successor_addr[DHT_SUCCESSOR_LIST_R - 1], 0, 256);
	memset(successor_hash[DHT_SUCCESSOR_LIST_R - 1], 0, SHA_DIGEST_LENGTH);
}