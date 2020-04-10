#include <stdlib.h>
#include "raft.h"
#include "client.h"

int main(int argc, char **argv) {
	FILE *fp = fopen("config", "r");
	if (fp == NULL) {
		fprintf(stderr, "can't read config file\n");
		fflush(stderr);
		exit(1);
	}
	raft_init_client(fp);
	fclose(fp);

	int i;
	for (i = 0; i < 20; ++i) {
		RAFT_DATA_TYPE data;
		sprintf(data.val, "sync_%d", i);
		int err = raft_put(&data, NULL, NULL, true);
		if (err < 0) {
			printf("failed to put %s\n", data.val);
		}
	}

	for (i = 0; i < 20; ++i) {
		RAFT_DATA_TYPE data;
		sprintf(data.val, "async_%d", i);
		unsigned long index, term;
		int err = raft_put(&data, &index, &term, false);
		if (err < 0) {
			printf("failed to put %s\n", data.val);
		}
	}
	return 0;
}

