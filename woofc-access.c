#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "uriparser2.h"


int WooFValidURI(char *str) 
{
	char *prefix;
	/*
	 * must begin with woof://
	 */
	prefix = strstr(str,"woof://");
	if(prefix == str) {
		return(1);
	} else {
		return(0);
	}
	
}

/*
 * extract namespace from full woof_name
 */
int WooFNameSpaceFromURI(char *woof_uri_str, char *woof_namespace, int len)
{
	struct URI uri;
	int i;

	if(!WooFValidURI(woof_uri_str)) { /* still might be local name, but return error */
		return(-1);
	}

	uri_parse2(woof_uri_str,&uri);
	if(uri.path == NULL) {
		return(-1);
	}
	/*
	 * walk back to the last '/' character
	 */
	i = strlen(uri.path);
	while(i >= 0) {
		if(uri.path[i] == '/') {
			if(i <= 0) {
				return(-1);
			}
			if(i > len) { /* not enough space to hold path */
				return(-1);
			}
			strncpy(woof_namespace,uri.path,i);
			return(1);
		}
		i--;
	}
	/*
	 * we didn't find a '/' in the URI path for the woofname -- error out
	 */
	return(-1);
}

int WooFNameFromURI(char *woof_uri_str, char *woof_name, int len)
{
	struct URI uri;
	int i;
	int j;

	if(!WooFValidURI(woof_uri_str)) { 
		return(-1);
	}

	uri_parse2(woof_uri_str,&uri);
	if(uri.path == NULL) {
		return(-1);
	}
	/*
	 * walk back to the last '/' character
	 */
	i = strlen(uri.path);
	j = 0;
	/*
	 * if last character in the path is a '/' this is an error
	 */
	if(uri.path[i] == '/') {
		return(-1);
	}
	while(i >= 0) {
		if(uri.path[i] == '/') {
			i++;
			if(i <= 0) {
				return(-1);
			}
			if(j > len) { /* not enough space to hold path */
				return(-1);
			}
			/*
			 * pull off the end as the name of the woof
			 */
			strncpy(woof_name,&(uri.path[i]),len);
			return(1);
		}
		i--;
		j++;
	}
	/*
	 * we didn't find a '/' in the URI path for the woofname -- error out
	 */
	return(-1);
} 

		
