#include "hw.h"
#include "woofc.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void split_string(const char* str, const char* delim) {
    	const char* start = str;
    	const char* end;
    	size_t delimiter_len = strlen(delim);

    	while ((end = strstr(start, delim)) != NULL) {
        	// Print substring from start to the delimiter
        	printf("%.*s\n", (int)(end - start), start);
        	start = end + delimiter_len;  // Move start pointer past the delimiter
    	}
    
    	// Print the last part (after the last delimiter)
    	if (*start) {
        	printf("%s\n", start);
    	}
}

int cjkHrw(WOOF* wf, unsigned long seq_no, void* ptr) {
    	HW_EL* el = (HW_EL*)ptr;
	int err;
    	fprintf(stdout, "[CJK] cjkHrw: received ping with message %s\n",el->string);
    	fprintf(stdout, "[CJK] cjkHrw: sending pong...\n");
	fflush(stdout);

	HW_EL new_el;
        char Wname[STRSIZE]; //woof that we will append to for pong
	memset(Wname,0,sizeof(Wname));
	/* el->string should hold: srcWoof_cjkHrw_destWoof 
	 * srcWoof is the one that triggered us via its append
	 * destWoof is the one we want to extract and append to to trigger the pong
	 * we will place destWoof in Wname
	 */

	/* find the DELIMITER string in el->string, error out if not found */
    	size_t delimiter_len = strlen(DELIMITER);
	const char* delim = strchr(el->string, *DELIMITER); 
	if (delim == NULL) { //delim points to starting char of DELIMITER
    		fprintf(stdout, "[CJK] cjkHrw: error delimiter not found instring %s (delim: %s)...\n",el->string, DELIMITER);
		fflush(stdout);
	    	return 1;
	}
	/* abc_xyz_efg: _xyz_ is delimiter, delim points to first _, delimiter_len=5
	 * start points to e, dest_len = 3
	 * strlen(start) is 3 (without \0; el->string assumed to be null terminated)
	 */
    	const char* start = delim + delimiter_len; 
	strncpy(Wname,start,strlen(start)); 
    	fprintf(stdout, "[CJK] cjkHrw: pong woof: %s\n",Wname);
	fflush(stdout);

	/* check if the woof exists */
	seq_no = WooFGetLatestSeqno(Wname);
	if (((int)seq_no) == -1) {
            	fprintf(stderr,"[CJK] cjkHrw: Unable to access %s\nYou must run cjk-init on the host where the woof lives first.\n", Wname);
	    	fflush(stderr);
	    	return 1;
	}

	/* perform a put using a string value of the current time */
	struct timespec ts;
	memset(new_el.string,0,sizeof(new_el.string));
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
    	if (err == -1) {
        	printf("[CJK] cjkHrw: Error getting the current time.\n");
        	return 1;
    	}
	snprintf(new_el.string, sizeof(new_el.string), "%lld", (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec);
  
        fprintf(stderr,"[CJK] cjkHrw: ponging %s in cjkHrw with ts %s\n",Wname,new_el.string);
        fflush(stderr);

	seq_no = WooFPut(Wname,"cjkHlw",(void *)&new_el);
	if(WooFInvalid(seq_no)) {
                fprintf(stderr,"[CJK] cjkHrw: pong WooFPut failed for %s\n",Wname);
                fflush(stderr);
                return 1;
        }
    	fprintf(stdout, "[CJK] cjkHrw: put succeeded with seq_no %ul\n",seq_no);
    	fprintf(stdout, "[CJK] cjkHrw: returning\n");
    	fflush(stdout);
	return 0;
}
