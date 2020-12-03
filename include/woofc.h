#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct woof_stc WOOF;

int WooFInit();
void WooFExit();

int WooFCreate(const char* name, unsigned long element_size, unsigned long history_size);

unsigned long WooFPut(const char* wf_name, const char* wf_handler, const void* element);
int WooFGet(const char* wf_name, void* element, unsigned long seq_no);

unsigned long WooFGetLatestSeqno(char* wf_name);
unsigned long WooFGetLatestSeqnoWithCause(char* wf_name,
                                          unsigned long cause_host,
                                          unsigned long long cause_seq_no,
                                          char* cause_woof_name,
                                          unsigned long cause_woof_latest_seq_no);

int WooFInvalid(unsigned long seq_no);

int WooFValidURI(const char* str);

#if defined (__cplusplus)
}
#endif
