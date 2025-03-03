#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/hmac.h>
#include <time.h>
#include <sys/time.h>

#include "woofc-access.h"
#include "woofc.h"
#include "woofc-caplets.h"
#include <stdbool.h>
#include "debug.h"

uint64_t WooFCapCheck(WCAP *cap, uint64_t key)
{
	unsigned char full_hmac[EVP_MAX_MD_SIZE];  // Store full HMAC
	unsigned int hmac_len;
	unsigned char *ukey = (unsigned char *)&key;
	unsigned char *udata = (unsigned char *)cap;
	int data_len = sizeof(WCAP);
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

	sprintf(cap_name,"%s.CAP",local_woof_name);

	err = WooFCreate(cap_name,sizeof(WCAP),WCAP_HISTORY);
	if(err < 0) {
		DEBUG_WARN("WooFCapInit: failed to create %s\n",cap_name);
		free(cap_name);
		return(-1);
	}

	// init principal cap
	// give it infinite lifetime
	capability.permissions = WCAP_PRINCIPAL;
	capability.frame_size = 0;
	capability.flags = 0;

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

WCAP *WooFCapAttenuate(WCAP *cap, uint32_t perm)
{
	WCAP *new_cap;
	uint32_t permitted = WCAP_MAX_PERM;
	// rules are listed in order of strength
	if(perm > cap->permissions) {
		return(NULL);  // cannot create  stronger cap
	}
	new_cap = (WCAP *)malloc(sizeof(WCAP));
	if(new_cap == NULL) {
		return(NULL);
	}
	memcpy(new_cap,cap,sizeof(WCAP));
printf("AT: start perms: %x %ld\n",new_cap->permissions, new_cap->check);
	permitted = new_cap->permissions;
	while(new_cap->permissions > perm) {
		permitted = permitted / 2;
		new_cap->permissions = permitted;
		new_cap->check = WooFCapCheck(new_cap,new_cap->check);
printf("AT: atten perms: %x %ld\n",new_cap->permissions, new_cap->check);
	}
printf("AT: final perms: %x %ld\n",new_cap->permissions, new_cap->check);
	new_cap->permissions = permitted;

	return(new_cap);

}

int WooFCapAuthorized(uint64_t secret, WCAP *cap, uint32_t perm)
{
	WCAP local;
	uint32_t permitted = WCAP_MAX_PERM;

	local.permissions = WCAP_PRINCIPAL;
	local.flags = 0;
	local.frame_size = 0;
	local.check = secret;

printf("AUTH %d: start perms: %x %x %ld\n",perm,local.permissions,permitted,local.check);
		while(permitted > perm) {
		permitted = permitted / 2;
		local.permissions = local.permissions / 2;
		local.check = WooFCapCheck(&local,local.check);
printf("AUTH %d: atten perms: %x %x %ld\n",perm,local.permissions,permitted,local.check);
	}
printf("AUTH %d: final perms: %x %x %ld\n",perm,local.permissions,permitted,local.check);

	if(local.check == cap->check) {
		return(1);
	} else {
		return(0);
	}
}

void WooFCapPrint(WCAP *cap)
{
	printf("permissions: %x\n",cap->permissions);
	printf("check: %ld\n",cap->check);
	return;
}
