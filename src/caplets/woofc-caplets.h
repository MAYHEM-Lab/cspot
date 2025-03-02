#ifndef WOOFC_CAPLETS_H
#define WOOFC_CAPLETS_H

#include <stdint.h>

struct cap_stc
{
	uint64_t permissions;
	uint64_t expiration; // units are seconds
	uint64_t check;
};

typedef struct cap_stc WCAP;

// in order of attenuation rules
#define WCAP_READ (0x1)
#define WCAP_WRITE (0x2)
#define WCAP_EXEC (0x4)
#define WCAP_INIT (0x8)
#define WCAP_DESTROY (0x16)
#define WCAP_POLICY_ATTACH (0x32)

#define WCAP_PRINCIPAL (WCAP_EXEC |\
			WCAP_WRITE |\
			WCAP_READ |\
			WCAP_INIT |\
			WCAP_DESTROY |\
			WCAP_POLICY_ATTACH)
	

#define WCAP_HISTORY (10) // max history of principal caps

uint64_t WooFCapCheck(WCAP *cap, uint64_t key);
int WooFCapInit(char *local_woof_name);
#endif
