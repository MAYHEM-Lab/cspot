#ifndef WOOFC_ACCESS_H
#define WOOFC_ACCESS_H

int WooFValidURI(char *str);
int WooFNameSpaceFromURI(char *woof_uri_str, char *woof_namespace, int len);
int WooFNameFromURI(char *woof_uri_str, char *woof_name, int len);
unsigned int WooFPortHash(char *namespace);
int WooFLocalIP(char *ip_str, int len);

unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element, unsigned long el_size);
int WooFMsgServer (char *namespace);

#endif

