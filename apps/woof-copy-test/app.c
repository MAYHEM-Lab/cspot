#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "woofc.h"
#include "woofc-host.h"

#define BACKUP_LOG_NAME "TESTBACKUP"
#define LOG_NAME "TEST"
#define LOG_SIZE 10000

void create_test_log(){

	unsigned long i;

	WooFCreate(LOG_NAME, sizeof(unsigned long), LOG_SIZE);

	for(i = 1; i <= LOG_SIZE; ++i){
		WooFPut(LOG_NAME, NULL, (void *)&i);
	}

}

int WooFTruncate(char *name, unsigned long seq_no) {
	WOOF *wf = WooFOpen(name);
	if (wf == NULL) {
		return -1;
	}
	WOOF_SHARED *wfs = wf->shared;

	P(&wfs->tail_wait);
	P(&wfs->mutex);

	if (seq_no == 0) {
		wfs->head = 0;
		wfs->tail = 0;
		wfs->seq_no = 1;
	} else {
		// if new seq_no is less than earliest seq_no, return -1
		unsigned long earliest_seqno = (wfs->tail + 1) % wfs->history_size;
		if (seq_no < earliest_seqno) {
			fprintf(stderr, "WooFTruncate: seq_no %lu is less than the earliest seq_no %lu\n", seq_no, earliest_seqno);
			fflush(stderr);
			V(&wfs->mutex);
			V(&wfs->tail_wait);
			WooFDrop(wf);
			return -1;
		}

		unsigned long latest_seqno = wfs->seq_no - 1;
		unsigned long trunc_head = (wfs->head + wfs->history_size - (latest_seqno - seq_no)) % wfs->history_size;
		wfs->head = trunc_head;
		wfs->seq_no = seq_no + 1;
	}
	
	V(&wfs->mutex);
	V(&wfs->tail_wait);
	WooFDrop(wf);

	return 1;
}

void copyWooFOptimized(char *original, char *copy, unsigned long element_size, unsigned long history_size, unsigned long upto){

	FILE *fp;
	WOOF *src;
	WOOF *dst;
	unsigned long idx;

	WooFCreate(copy, element_size, history_size);

	src = WooFOpen(original);
	dst = WooFOpen(copy);
	idx = WooFIndexFromSeqno(src, 1);
	
	WooFReplace(dst, src, idx, LOG_SIZE);
	
	dst->shared->head = src->shared->head;
	dst->shared->tail = src->shared->tail;
	dst->shared->seq_no = src->shared->seq_no;

	WooFTruncate(copy, upto);

	fp = fopen("debug", "w");
	WooFPrintMeta(fp, LOG_NAME);
	fclose(fp);

}

void copyWooFSystem(char *original, char *copy, unsigned long element_size, unsigned long history_size, unsigned long upto){

	char cmd[256];

	sprintf(cmd, "cp %s %s", original, copy);
	system(cmd);
	WooFTruncate(copy, upto);

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

/* microseconds */
unsigned long get_elapsed_time(struct timeval ts_start, struct timeval ts_end){

	return (ts_end.tv_sec * 1000000 + ts_end.tv_usec) - (ts_start.tv_sec * 1000000 + ts_start.tv_usec);

}

int copyWooFNaive(char *original, char *copy, unsigned long element_size, unsigned long history_size, unsigned long upto){

	int status;
	unsigned long idx;
	unsigned long last;
	unsigned long write_status;
	void *buffer;

	status = WooFCreate(copy, element_size, history_size);
	if(status < 0){
		fprintf(stdout, "copyWooF: failed to create %s\n", copy);
		fflush(stdout);
		return 0;
	}

	last = WooFGetLatestSeqno(original);
	if(last == (unsigned long)-1){
		if(! (access(original, F_OK) == 0)){ /* woof not present */
			status = WooFCreate(original, element_size, history_size);
			if(status < 0){
				fprintf(stdout, "copyWooF: failed to create %s\n", original);
				fflush(stdout);
				return 0;
			}
			last = 0;
		}else{
			fprintf(stdout, "copyWooF: failed to get last seq no of %s\n", original);
			fflush(stdout);
			return 0;
		}
	}

	if((upto != (unsigned long)-1) && upto <= last){
		last = upto;
	}

	buffer = malloc(element_size);

	for(idx = 1; idx <= last; ++idx){
		status = WooFGet(original, buffer, idx);
		if(status < 0){
			fprintf(stdout, "copyWooF: failed to read %s at %lu\n", original, idx);
			fflush(stdout);
			return 0;
		}
		write_status = WooFPut(copy, NULL, buffer);
		if(write_status == (unsigned long)-1){
			fprintf(stdout, "copyWooF: failed to insert %s at %lu\n", copy, idx);
			fflush(stdout);
			return 0;
		}
	}

	free(buffer);

	return 1;

}

void validate(unsigned long upto){

	unsigned long latest;
	unsigned long idx;
	unsigned long val;

	latest = WooFGetLatestSeqno(BACKUP_LOG_NAME);
	if(latest != upto){
		fprintf(stdout, "validation failed\n");
		fflush(stdout);
		exit(1);
	}

	for(idx = 1; idx <= latest; ++idx){
		WooFGet(BACKUP_LOG_NAME, (void *)&val, idx);
		if(val != idx){
			fprintf(stdout, "validation failed\n");
			fflush(stdout);
			exit(1);
		}
	}

}

int main(int argc, char *argv[]){

	WooFInit();
	struct timeval ts_start;
	struct timeval ts_end;
	unsigned long upto;

	upto = (LOG_SIZE / 100) * 80;

	create_test_log();

	gettimeofday(&ts_start, NULL);
	copyWooFOptimized(LOG_NAME, BACKUP_LOG_NAME, sizeof(unsigned long), LOG_SIZE, upto);
	gettimeofday(&ts_end, NULL);
	validate(upto);
	fprintf(stdout, "optimized copy: %lu\n", get_elapsed_time(ts_start, ts_end));

	gettimeofday(&ts_start, NULL);
	copyWooFSystem(LOG_NAME, BACKUP_LOG_NAME, sizeof(unsigned long), LOG_SIZE, (LOG_SIZE / 100) * 80);
	gettimeofday(&ts_end, NULL);
	validate(upto);
	fprintf(stdout, "system copy: %lu\n", get_elapsed_time(ts_start, ts_end));

	gettimeofday(&ts_start, NULL);
	copyWooFNaive(LOG_NAME, BACKUP_LOG_NAME, sizeof(unsigned long), LOG_SIZE, (LOG_SIZE / 100) * 80);
	gettimeofday(&ts_end, NULL);
	validate(upto);
	fprintf(stdout, "naive copy: %lu\n", get_elapsed_time(ts_start, ts_end));

	return 0;

}
