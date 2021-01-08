#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"

#define BACKUP_LOG_NAME "TESTBACKUP"
#define LOG_NAME "TEST"
#define LOG_SIZE 10

void create_test_log(){

	unsigned long i;

	WooFCreate(LOG_NAME, sizeof(unsigned long), LOG_SIZE);

	for(i = 1; i <= LOG_SIZE; ++i){
		WooFPut(LOG_NAME, NULL, (void *)&i);
	}

}

void copyWooFOptimized(char *original, char *copy, unsigned long element_size, unsigned long history_size, unsigned long upto){

	WOOF *src;
	WOOF *dst;
	unsigned long idx;

	WooFCreate(copy, element_size, history_size);

	src = WooFOpen(original);
	dst = WooFOpen(copy);
	idx = WooFIndexFromSeqno(src, 1);
	
	WooFReplace(dst, src, idx, upto);

	memcpy(dst->shared, src->shared, sizeof(WOOF_SHARED));
	memcpy(dst->mio, src->mio, sizeof(MIO));
	strcpy(dst->shared->filename, copy);
	dst->shared->seq_no = upto;
	dst->shared->head = upto;
	//dst->shared->tail = upto;

}

void print_log(char *name){

	unsigned long latest;
	unsigned long i;
	unsigned long val;

	latest = WooFGetLatestSeqno(name);
	for(i = 1; i <= latest; ++i){
		WooFGet(name, (void *)&val, i);
		fprintf(stdout, "got %lu at %lu\n", val, i);
	}

}

int main(int argc, char *argv[]){

	WooFInit();

	create_test_log();
	copyWooFOptimized(LOG_NAME, BACKUP_LOG_NAME, sizeof(unsigned long), LOG_SIZE, LOG_SIZE - 1);

	fprintf(stdout, "printing test...\n");
	print_log(LOG_NAME);

	fprintf(stdout, "printing copy...\n");
	print_log(BACKUP_LOG_NAME);

	return 0;

}
