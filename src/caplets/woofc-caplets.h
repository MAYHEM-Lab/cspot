#ifndef WOOFC_CAPLETS_H
#define WOOFC_CAPLETS_H

#include <stdint.h>

struct cap_stc
{
	uint32_t permissions;
	uint16_t flags;
	uint16_t frame_size;
	uint64_t check;
};

typedef struct cap_stc WCAP;

// in order of attenuation rules
#define WCAP_READ (0x1)
#define WCAP_WRITE (0x2)
#define WCAP_EXEC (0x4)
#define WCAP_INIT (0x8)
#define WCAP_DESTROY (0x10)
#define WCAP_POLICY_ATTACH (0x20)

#define WCAP_MAX_PERM WCAP_POLICY_ATTACH // set to largest perm

#define WCAP_PRINCIPAL (WCAP_EXEC |\
			WCAP_WRITE |\
			WCAP_READ |\
			WCAP_INIT |\
			WCAP_DESTROY |\
			WCAP_POLICY_ATTACH)
	

#define WCAP_HISTORY (10) // max history of principal caps

uint64_t WooFCapCheck(WCAP *cap, uint64_t key);
int WooFCapInit(char *local_woof_name);
WCAP *WooFCapAttenuate(WCAP *cap, uint32_t perm);
int WooFCapAuthorized(uint64_t secret, WCAP *cap, uint32_t perm);
void WooFCapPrint(char *woof_name, WCAP *cap);
#endif
