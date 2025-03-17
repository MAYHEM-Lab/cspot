#include "common.hpp"

#include "debug.h"

#include <fmt/format.h>
#include <woofc-access.h>
#include <woofc-caplets.h>

namespace cspot::mqtt {

int WooFGatewayIP(char* ip_str, int len)
{
        struct ifaddrs* addrs;
        struct ifaddrs* tmp;
        char* local_ip;

        /*
         * the Host_ip is the IP address of the woofc-mqtt-gateway
         *
         * It has to be set for this device
         */
        if (Host_ip[0] != 0) {
                strncpy(ip_str, Host_ip, len);
                return (1);
        }

        /*
         * could be an environment variable
         */
        local_ip = getenv("WOOFC_IP");
        if (local_ip != NULL) {
                strncpy(ip_str, local_ip, len);
                return (1);
        }

        /*
         * otherwise, use localhost
         */
        strncpy(ip_str, "127.0.0.1", len);
        return(1);

}

int WooFGetMQTT(const char *wf_name, const void* element, const unsigned long seqno, WCAP *cap)
{
	char gateway_ip[IPLEN];
	char topic[1024];
	char user[256];
	char pw[256];
	FILE *fd;
	int err;
	char pub_string[1024*17];
	char sub_string[256];
	char cap_str[1024];
	char reply_string[256];
	int code;
	int s;
	TXL *tl;
	int lsize;
	

	/*
	 * get the gateway IP
	 */
	err = WooFGatewayIP(gateway_ip,sizeof(gateway_ip));
	if(err < 0) {
		return(-1);
	}
	
	err = WooFCredentials(user,sizeof(user),pw,sizeof(pw));
	if(err < 0) {
		return(-1);
	}


	memset(topic,0,sizeof(topic));
	err = MakeMQTTTopic(topic,sizeof(topic));
	if(err < 0) {
		return(-1);
	}

	/*
	 * set up the subscription for the reply
	 */
	memset(sub_string,0,sizeof(sub_string));
	sprintf(sub_string,"/usr/bin/mosquitto_sub -C 1 -h %s -t %s.input -u \'%s\' -P \'%s\'",
		gateway_ip,
		topic,
		user,
		pw);
	fd = popen(sub_string,"r");
	if(fd == NULL) {
		fprintf(stderr,"WooFGetMQTT: mosquitto_sub failed\n");
		return(-1);
	}

	/*
	 * now do the send side
	 */

	memset(pub_string,0,sizeof(pub_string));
	if(cap == NULL) {
		sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d|%d\'",
			gateway_ip,
			topic,
			user,
			pw,
			wf_name,
			WOOF_MQTT_GET,
			seqno);
	} else {
		(void)ConvertBinarytoASCII(cap_str,cap,sizeof(WCAP));
		sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d|%d|%s\'",
			gateway_ip,
			topic,
			user,
			pw,
			wf_name,
			WOOF_MQTT_GET_CAP,
			seqno,
			cap_str);
	}
	/*
	 * publish the get
	 */
printf("pub_string: %s\n",pub_string);
	system(pub_string);

	/*
	 * now wait for the reply
	 * FIXME: should use select() so that it can time out the read
	 */
	memset(reply_string,0,sizeof(reply_string));
	s = read(fileno(fd),reply_string,sizeof(reply_string));
printf("read: %d\n",s);
	pclose(fd);
	if(s > 0) {
		tl = ParseLine(reply_string,"|");
		if((tl == NULL) || (tl->list->first == NULL) ||
		   (tl->list->first->next == NULL) || (tl->list->first->next->next == NULL) ||
		   (tl->list->first->next->next->next == NULL)) {
			fprintf(stderr,
				"WooFGetMQTT: bad reply: %s\n",reply_string);
			if(tl != NULL) {
				DestroyTXL(tl);
			}
			return(-1);
		}
		code = atoi(tl->list->first->next->value.s);
		if(code != WOOF_MQTT_GET_RESP) {
			fprintf(stderr,"WooFGetMQTT: bad resp code %d (should be %d)\n",
					code,
					WOOF_MQTT_GET_RESP);
			DestroyTXL(tl);
			return(-1);
		}
		lsize = atoi(tl->list->first->next->next->value.s);
		if(lsize <= 0) {
			DestroyTXL(tl);
			return(-1);
		}
		/*
		 * this is risky because this call can't tell whether there is enough space to hold an element.  To
		 * do so, it would need to call GetElSize, but the caller probably did this and the gateway will as well
		 */
		(void)ConvertASCIItoBinary((unsigned char *)element,tl->list->first->next->next->next->value.s,lsize);
		DestroyTXL(tl);
		return(1);
	}
	return(-1);
}
} // namespace cspot::zmq
