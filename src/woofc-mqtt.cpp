#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <czmq.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "uriparser2.h"

#include "woofc.h" /* for WooFPut */
#include "woofc-access.h"
#include "woofc-mqtt.h"

int ConvertASCIItoBinary(unsigned char *dest, char *src, int len)
{
	int count;
	char *curr;
	unsigned char c;
	unsigned char high;
	unsigned char low;

	/*
	 * assumes data is encoded as 112233445566778899AABBCCDDEEFF in 2 char hex
	 */

	curr = src;
	count = 0;
	while((count < len) && (*curr != 0)) {
		if((curr[0] >= '0') && (curr[0] <= '9')) {
			high = (curr[0] - '0') * 16;
		} else if ((curr[0] >= 'a') && (curr[0] <= 'f')) {
			high = (curr[0] - 'a' + 10) * 16;
		}
		if((curr[1] >= '0') && (curr[1] <= '9')) {
			low = (curr[1] - '0');
		} else if ((curr[1] >= 'a') && (curr[1] <= 'f')) {
			low = (curr[1] - 'a' + 10);
		}
		c = high+low;
		dest[count] = c;
		curr += 2;
		count++;
	}
	return(1);
}

void FreeWMQTT(WMQTT *wm)
{
	if(wm == NULL) {
		return;
	}
	if(wm->element != NULL) {
		free(wm->element);
	}
	if(wm->handler_name != NULL) {
		free(wm->handler_name);
	}
	free(wm);
	return;
}
/*
 * format is woof_name:command:args...
 */

WMQTT *ParseMQTTString(char *str)
{
	WMQTT *wm;
	TXL *tl;
	int command;
	int len;
	int blen;
	int err;

	wm = (WMQTT *)malloc(sizeof(WMQTT));
	if(wm == NULL) {
		return(NULL);
	}
	memset(wm,0,sizeof(WMQTT));;
	tl = ParseLine(str,"|");
	if((tl == NULL) || (tl->list == NULL) ||
			(tl->list->first == NULL) || (tl->list->first->next == NULL)) {
		free(wm);
		if(tl != NULL) {
			DestroyTXL(tl);
		}
		return(NULL);
	}
	/*
	 * get the woof name
	 */
	strncpy(wm->woof_name,tl->list->first->value.s,sizeof(wm->woof_name));

	/*
	 * get the command
	 */
	wm->command = atoi(tl->list->first->next->value.s);

	/*
	 * strings are operation dependent
	 */
	switch(wm->command) {
		case WOOF_MQTT_PUT:
			/* next is handler name (or NULL) */
			if((tl->list->first->next->next == NULL) ||
			   (tl->list->first->next->next->next == NULL)) {
				FreeWMQTT(wm);
				DestroyTXL(tl);
				return(NULL);
			}
			if(strcmp(tl->list->first->next->next->value.s,"NULL") != 0) {
				len = strlen(tl->list->first->next->next->value.s)+1;
				wm->handler_name = (char *)malloc(len);
				if(wm->handler_name == NULL) {
					FreeWMQTT(wm);
					DestroyTXL(tl);
					return(NULL);
				}
				memset(wm->handler_name,0,len);
				strncpy(wm->handler_name,tl->list->first->next->next->value.s,len-1);
			}

			/*
			 * the element (encoded as 2char hex ascii) is next
			 */
			blen = strlen(tl->list->first->next->next->next->value.s) / 2;
			while((blen%4) != 0) {
				blen++;
			}
			wm->element = (unsigned char *)malloc(blen);
			if(wm->element == NULL) {
				FreeWMQTT(wm);
				DestroyTXL(tl);
				return(NULL);
			}
			memset(wm->element,0,blen);
			err = ConvertASCIItoBinary(wm->element,tl->list->first->next->next->next->value.s,blen);
			if(err < 0) {
				FreeWMQTT(wm);
				DestroyTXL(tl);
				return(NULL);
			}
			break;
		case WOOF_MQTT_GET_EL_SIZE:
			/*
			 * we only need the woof name and the code
			 */
			break;
		default:
			FreeWMQTT(wm);
			DestroyTXL(tl);
			return(NULL);
	}

	return(wm);
}



