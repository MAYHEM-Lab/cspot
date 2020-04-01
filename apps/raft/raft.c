#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raft.h"

FILE *raft_log_output;
int raft_log_level;

int random_timeout(unsigned long seed) {
	srand(seed);
	return RAFT_TIMEOUT_MIN + (rand() % (RAFT_TIMEOUT_MAX - RAFT_TIMEOUT_MIN));
}

unsigned long get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec / 1000;
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

void log_set_level(int level)
{
	raft_log_level = level;
}

void log_set_output(FILE *file)
{
	raft_log_output = file;
	// log_set_level(LOG_DEBUG);
}

void log_debug(const char *tag, const char *message)
{
	if (raft_log_level > LOG_DEBUG)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(raft_log_output, "\033[0;32m");
	fprintf(raft_log_output, "DEBUG| %.19s:%.3d [%s]: %s\n", ctime(&now), get_milliseconds()% 1000, tag, message);
	fprintf(raft_log_output, "\033[0m");
}

void log_info(const char *tag, const char *message)
{
	if (raft_log_level > LOG_INFO)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(raft_log_output, "\033[0;34m");
	fprintf(raft_log_output, "INFO | %.19s:%.3d [%s]: %s\n", ctime(&now), get_milliseconds()% 1000, tag, message);
	fprintf(raft_log_output, "\033[0m");
}

void log_warn(const char *tag, const char *message)
{
	if (raft_log_level > LOG_WARN)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(raft_log_output, "\033[0;33m");
	fprintf(raft_log_output, "WARN | %.19s:%.3d [%s]: %s\n", ctime(&now), get_milliseconds()% 1000, tag, message);
	fprintf(raft_log_output, "\033[0m");
}

void log_error(const char *tag, const char *message)
{
	if (raft_log_level > LOG_ERROR)
	{
		return;
	}
	time_t now;
	time(&now);
	fprintf(raft_log_output, "\033[0;31m");
	fprintf(raft_log_output, "ERROR| %.19s:%.3d [%s]: %s\n", ctime(&now), get_milliseconds()% 1000, tag, message);
	fprintf(raft_log_output, "\033[0m");
}