#ifndef WOOFC_ACCESS_H
#define WOOFC_ACCESS_H

int WooFValidURI(char *str);
int WooFNameSpaceFromURI(char *woof_uri_str, char *woof_namespace, int len);
int WooFNameFromURI(char *woof_uri_str, char *woof_name, int len);

#endif

