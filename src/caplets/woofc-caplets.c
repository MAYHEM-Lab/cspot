#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <openssl/hmac.h>
#include <time.h>
#include <sys/time.h>

#include "woofc-access.h"
#include "woofc.h"

uint64_t WooFCapCheck(WCAP *cap, uint64_t key)
{
	unsigned char full_hmac[EVP_MAX_MD_SIZE];  // Store full HMAC
	unsigned int hmac_len;
	unsigned char *ukey = (unsigned char *)&key;
	unsigned char *udata = (unsigned char *)cap;
	int data_len = 2 * sizeof(uint64_t); // do not include check field
	uint64_t hm;

	// Compute full HMAC using SHA-256
	HMAC(EVP_sha256(), ukey, sizeof(key), udata, data_len, full_hmac, &hmac_len);

	// Truncate to 64 bits (first 8 bytes)
	memcpy(&hm, full_hmac, sizeof(hm));

	return(hm);
}

// create cap woof from woof name
// must be done locally
int WooFCapInit(char *local_woof_name)
{
	int err;
	int cap_name_len = strlen(local_woof_name)+strlen(".CAP")+1;
	char *cap_name;
	WCAP capability;
	uint64_t nonce;
	struct timeval tm;
	unsigned long seq_no;

	cap_name = (char *)malloc(cap_name_len);
	if(cap_name == NULL) {
		return(-1);
	}

	err = WooFCreate(cap_name,sizeof(WCAP),WCAP_HISTORY);
	if(err < 0) {
		DEBUG_WARN("WooFCapInit: failed to create %s\n",cap_name);
		free(cap_name);
		return(-1);
	}

	// init principal cap
	// give it infinite lifetime
	capability.permissions = WCAP_PRINCIPAL;
	capability.expiration = 0;

	// compute random nonce for principal secret
	gettimeofday(&tm,NULL);
	srand48(tm.tv_sec+tm.tv_usec);
	nonce = (uint64_t)(drand48() * (double)(0xFFFFFFFFFFFFFFFF));
	capability.check = nonce;

	// put the cap
	seq_no = WooFPut(cap_name,NULL,(unsigned char *)&capability);
	if(seq_no == (unsigned long)-1) {
		DEBUG_WARN("WooFCapInit: failed to put prinicpal to %s\n",
			cap_name);
		free(cap_name);
		return(-1);
	}

	free(cap_name);
	return(0);
}
	
	
	
	
	
	// 

		

	

	


