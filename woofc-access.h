#ifndef WOOFC_ACCESS_H
#define WOOFC_ACCESS_H

int WooFValidURI(char *str);
int WooFNameSpaceFromURI(char *woof_uri_str, char *woof_namespace, int len);
int WooFNameFromURI(char *woof_uri_str, char *woof_name, int len);
int WooFIPAddrFromURI(char *woof_uri_str, char *woof_ip, int len);
unsigned int WooFPortHash(char *namespace);
int WooFLocalIP(char *ip_str, int len);

unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element, unsigned long el_size);
unsigned long WooFMsgGetElSize(char *woof_name);
int WooFMsgServer (char *namespace);

int WooFURINameSpace(char *woof_uri_str, char *woof_namespace, int len);

/*
 * 2 minute timeout
 */
#define WOOF_MSG_REQ_TIMEOUT (120000)

#define WOOF_MSG_THREADS (5)

#define WOOF_MSG_PUT (1)
#define WOOF_MSG_GET_EL_SIZE (2)
#define WOOF_MSG_GET (3)
#define WOOF_MSG_GET_TAIL (4)
#define WOOF_MSG_GET_LATEST_SEQNO (5)

#define WOOF_MSG_CACHE_SIZE (100)

#endif

