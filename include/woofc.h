#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#define CSPOT_VERSION (3.0)

typedef struct woof_stc WOOF;

int WooFInit();
void WooFSetInit();
void WooFExit();

int WooFCreate(const char* name, unsigned long element_size, unsigned long history_size);
int WooFSetSeqno(char *name, unsigned long new_seqno); // for reset

unsigned long WooFPut(const char* wf_name, const char* wf_handler, const void* element);
int WooFGet(const char* wf_name, void* element, unsigned long seq_no);
int WooFGetRange(const char* wf_name, void* elements,
		unsigned long seq_no, unsigned int count);

unsigned long WooFGetLatestSeqno(const char* wf_name);
unsigned long WooFGetEarliestSeqno(const char* wf_name);
unsigned long WooFGetLatestSeqnoWithCause(const char* wf_name,
                                          unsigned long cause_host,
                                          unsigned long long cause_seq_no,
                                          const char* cause_woof_name,
                                          unsigned long cause_woof_latest_seq_no);

unsigned long WooFGetNameID();

int WooFInvalid(unsigned long seq_no);

int WooFValidURI(const char* str);

int WooFLocalIP(char* ip_str, int len);

#define WOOFNAMESIZE (256)
unsigned long WooFGetElSize(WOOF* wf, const char* wf_name);

const char* WooFGetFileName(WOOF* wf);
#define WoofGetFileName WooFGetFileName

extern int WooF_is_server; // for signal installation


#if defined (__cplusplus)
}
#endif
