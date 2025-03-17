#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <uriparser2.h>
#include <textlist.h>
#include <woofc-mqtt.h>
#include <woofc-device-linux.h>
#include <woofc-device-cspot.h>

#define IPLEN (17)
static char Host_ip[IPLEN]; /* IP of woofc-mqtt-gateway */
static char Device_name[1024]; /* string identifying this device */
static char User_name[256];
static char Password[256];

int WooFInvalid(unsigned long seq_no) {
    if (seq_no == (unsigned long)-1) {
        return (1);
    } else {
        return (0);
    }
}

int WooFValidURI(const char* str) {
    /*
     * must begin with woof://
     */
    char *prefix = strstr(str, "woof://");
    if (prefix == str) {
        return (1);
    } else {
        return (0);
    }
}

/*
 * convert URI to namespace path (C version)
 */
int WooFURINameSpace(char* woof_uri_str, char* woof_namespace, int len) 
{
	URI *uri;

	if (!WooFValidURI(woof_uri_str)) { /* still might be local name, but return error */
	return -1;
	}

	uri = uri_parse(woof_uri_str);
	if(uri == NULL) {
	    return -1;
	}

	if(uri->path == NULL) {
		free(uri);
		return(-1);
	}

	strncpy(woof_namespace, uri->path, len);
	free(uri);

	return 1;
}


/*
 * extract namespace from full woof_name (C version)
 */
int WooFNameSpaceFromURI(const char* woof_uri_str, char* woof_namespace, int len) 
{
	URI *uri;
	int i;

	if (!WooFValidURI(woof_uri_str)) { /* still might be local name, but return error */
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if(uri == NULL) {
		return -1;
	}

	if(uri->path == NULL) {
		free(uri);
		return -1;
	}
	/*
	* walk back to the last '/' character
	*/
	i = strlen(uri->path);
	while (i >= 0) {
		if (uri->path[i] == '/') {
		    if (i <= 0) {
			free(uri);
			return (-1);
		    }
		    if (i > len) { /* not enough space to hold path */
			free(uri);
			return (-1);
		    }
		    strncpy(woof_namespace, uri->path, i);
		    free(uri);
		    return (1);
		}
		i--;
	}
	/*
	* we didn't find a '/' in the URI path for the woofname -- error out
	*/
	free(uri);
	return (-1);
}

int WooFNameFromURI(const char* woof_uri_str, char* woof_name, int len) 
{
	URI *uri;
	int i;
	int j;
	char *prefix = strstr(woof_uri_str,"woof://");

	if(prefix == NULL) {
		strncpy(woof_name,woof_uri_str,len);
		return(1);
	}

	if (!WooFValidURI(woof_uri_str)) {
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if(uri == NULL) {
		return -1;
	}

	if(uri->path == NULL) {
		free(uri);
		return -1;
	}
	/*
	* walk back to the last '/' character
	*/
	i = strlen(uri->path);
	j = 0;
	/*
	* if last character in the path is a '/' this is an error
	*/
	if (uri->path[i] == '/') {
		free(uri);
		return (-1);
	}

	while (i >= 0) {
		if (uri->path[i] == '/') {
		    i++;
		    if (i <= 0) {
			free(uri);
			return (-1);
		    }
		    if (j > len) { /* not enough space to hold path */
			free(uri);
			return (-1);
		    }
		    /*
		     * pull off the end as the name of the woof
		     */
		    strncpy(woof_name, &(uri->path[i]), len);
		    free(uri);
		    return (1);
		}
		i--;
		j++;
	}
	/*
	* we didn't find a '/' in the URI path for the woofname -- error out
	*/
	free(uri);
	return (-1);
}

/*
 * returns IP address to avoid DNS issues
 */
int WooFIPAddrFromURI(const char* woof_uri_str, char* woof_ip, int len) 
{
	URI *uri;
	int err;
	struct hostent* he;
	struct in_addr** list;
	struct in_addr addr;

	if (!WooFValidURI(woof_uri_str)) {
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if(uri == NULL) {
		return -1;
	}

	if(uri->host == NULL) {
		free(uri);
		return -1;
	}

	/*
	* test to see if the host is an IP address already
	*/
	err = inet_aton(uri->host, &addr);
	if (err == 1) {
		strncpy(woof_ip, uri->host, len);
		free(uri);
		return (1);
	}

	/*
	* here, assume it is a DNS name
	*/
	he = gethostbyname(uri->host);
	if (he == NULL) {
		free(uri);
		return (-1);
	}

	list = (struct in_addr**)he->h_addr_list;
	if (list == NULL) {
		free(uri);
		return (-1);
	}

	for (int i = 0; list[i] != NULL; i++) {
		/*
		 * return first non-NULL ip address
		 */
		if (list[i] != NULL) {
		    strncpy(woof_ip, inet_ntoa(*list[i]), len);
		    free(uri);
		    return (1);
		}
	}

	free(uri);
	return (-1);
}

int WooFPortFromURI(const char* woof_uri_str, int* woof_port) 
{
	URI *uri;

	if (!WooFValidURI(woof_uri_str)) {
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if(uri == NULL) {
		return(-1);
	}

	if (uri->port == 0) {
		free(uri);
		return -1;
	}

	if (uri->port == -1) {
		free(uri);
		return (-1);
	}

	*woof_port = (int)uri->port;
	free(uri);

	return (1);
}

int WooFLocalIP(char* ip_str, int len) 
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

int WooFDeviceName(char *device_name, int len)
{
	char *e_dev;
	/*
	 * the Host_ip is the IP address of the woofc-mqtt-gateway
	 *
	 * It has to be set for this device
	 */
	if (Device_name[0] != 0) {
		strncpy(device_name, Device_name, len);
		return (1);
	}

	/*
	 * could be an environment variable
	 */
	e_dev = getenv("WOOFC_DEVICE_NAME");
	if (e_dev != NULL) {
		strncpy(device_name, e_dev, len);
		return (1);
	}

	/*
	 * otherwise, error out (could be gethostname() though)
	 */
	return(-1);

}

int WooFCredentials(char *user, int ulen, char *pw, int pwlen)
{
	char *cred;
	/*
	 * the Host_ip is the IP address of the woofc-mqtt-gateway
	 *
	 * It has to be set for this device
	 */
	if ((User_name[0] != 0) && (Password[0] != 0)) {
		strncpy(user, User_name, ulen);
		strncpy(pw, Password, pwlen);
		return(1);
	}

	/*
	 * could be an environment variable
	 */
	cred = getenv("WOOFC_MQTT_USER");
	if (cred == NULL) {
		fprintf(stderr,"no user name for MQTT broker\n");
		fprintf(stderr,"try setting WOOFC_MQTT_USER environment variable to user name\n");
		return (-1);
	}
	strncpy(User_name, cred, ulen);
	strncpy(user, cred, ulen);
	cred = getenv("WOOFC_MQTT_PW");
	if (cred == NULL) {
		fprintf(stderr,"no password for MQTT broker\n");
		fprintf(stderr,"try setting WOOFC_MQTT_PW environment variable to the password name\n");
		return (-1);
	}
	strncpy(Password, cred, pwlen);
	strncpy(pw, cred, pwlen);

	/*
	 * otherwise, error out (could be gethostname() though)
	 */
	return(1);

}

int WooFLocalName(const char* woof_name, char* local_name, int len) 
{
	return (WooFNameFromURI(woof_name, local_name, len));
}

int IsRemoteWooFOld(const char* wf_name)
{
	char my_ip[IPLEN];
	char uri_ip[IPLEN];
	int err;

	memset(my_ip,0,sizeof(my_ip));
	err = WooFLocalIP(my_ip,sizeof(my_ip));
	if(err < 0) {
		return(0);
	}
	memset(uri_ip,0,sizeof(uri_ip));
	err = WooFIPAddrFromURI(wf_name,uri_ip,sizeof(uri_ip));
	if(err < 0) {
		return(0);
	}
	if(strncmp(my_ip,uri_ip,sizeof(my_ip)) == 0) {
		return(0);
	} else {
		return(1);
	}
}

/*
 * device woof name format
 * woof://gateway_ip/device_name/woof_file_name
 */
int IsRemoteWooF(const char* wf_name)
{
	TXL *tl;
	char gateway_ip[IPLEN];
	char device_name[1024];
	int err;

	tl = ParseLine(wf_name,"/");
	if(tl == NULL) {
		return(1);
	}

	/*
	 * if the string is not a URI, consider it a local name
	 */
	if(FindWord(tl,"woof:") == NULL) {
		DestroyTXL(tl);
		return(0);
	}

	err = WooFLocalIP(gateway_ip,sizeof(gateway_ip));
	if(err < 0) {
		DestroyTXL(tl);
		return(1);
	}
	/*
	 * just look for them (not in any position))
	 *
	 * FIXME: should be second and third positions
	 */
	if(FindWord(tl,gateway_ip) == NULL) {
		DestroyTXL(tl);
		return(1);
	}

	err = WooFDeviceName(device_name,sizeof(device_name));
	if(err < 0) {
		DestroyTXL(tl);
		return(1);
	}
	if(FindWord(tl,device_name) == NULL) {
		DestroyTXL(tl);
		return(1);
	}
	DestroyTXL(tl);
	return(0);
}

int MakeMQTTTopic(char *topic, int len)
{
	char gateway_ip[IPLEN];
	char device_name[1024];
	int err;

	err = WooFLocalIP(gateway_ip,sizeof(gateway_ip));
	if(err < 0) {
		fprintf(stderr,"MakeMQTTTopic: ERROR -- gateway IP name not found\n");
		fflush(stderr);
		return(-1);
	}


	err = WooFDeviceName(device_name,sizeof(device_name));
	if(err < 0) {
		fprintf(stderr,"MakeMQTTTopic: ERROR -- local device name not found\n");
		fflush(stderr);
		return(-1);
	}

	sprintf(topic,"/%s/%s",gateway_ip,device_name);
	return(1);
}

unsigned long WooFGetElSizeMQTT(const char *wf_name)
{
	char gateway_ip[IPLEN];
	char user[256];
	char pw[256];
	char topic[1024];
	FILE *fd;
	char pub_string[2048];
	char sub_string[2048];
	char reply_string[256];
	int size;
	int code;
	int s;
	int err;
	TXL *tl;

	err = WooFLocalIP(gateway_ip,sizeof(gateway_ip));
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
	 * set up subscription
	 */
	memset(sub_string,0,sizeof(sub_string));
	sprintf(sub_string,"/usr/bin/mosquitto_sub -C 1 -h %s -t %s.input -u \'%s\' -P \'%s\'",
			gateway_ip,
			topic,
			user,
			pw);
	fd = popen(sub_string,"r");
	if(fd == NULL) {
		fprintf(stderr,"WooFGetElSizeMQTT: mosquitto_sub failed\n");
		return(-1);
	}

	/*
	 * make the request
	 */
	sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d\'",
		gateway_ip,
		topic,
		user,
		pw,
		wf_name,
		WOOF_MQTT_GET_EL_SIZE);
	system(pub_string);
printf("WooFGetElSizeMQTT: published reply %s\n",pub_string);

	/*
	 * now wait for the reply
	 * FIXME: should use select() so that it can time out the read
	 */
	memset(reply_string,0,sizeof(reply_string));
	s = read(fileno(fd),reply_string,sizeof(reply_string));
	pclose(fd);
printf("WooFGetElSizeMQTT: got reply %s\n",reply_string);
	if(s > 0) {
		/*
		 * the topic, here, is really the woof_name -- used it to save space
		 */
		tl = ParseLine(reply_string,"|");
		if((tl == NULL) || (tl->list == NULL) ||
		(tl->list->first->next == NULL) || (tl->list->first->next->next == NULL)) {
			fprintf(stderr,
				"WooFGetElSizeMQTT: bad reply: %s\n",reply_string);
			if(tl != NULL) {
				DestroyTXL(tl);
			}
			return(-1);
		}
		code = atoi(tl->list->first->next->value.s);
		if(code != WOOF_MQTT_GET_EL_SIZE_RESP) {
			fprintf(stderr,"WooFGetElSizeMQTT: bad resp code %d (should be %d)\n",
					code,
					WOOF_MQTT_GET_EL_SIZE_RESP);
			DestroyTXL(tl);
			return(-1);
		}
		size = atoi(tl->list->first->next->next->value.s);
		DestroyTXL(tl);
		return((unsigned long)size);
	}
	return(-1);
}

unsigned long WooFGetLatestSeqnoMQTT(const char* wf_name)
{
	char gateway_ip[IPLEN];
	char user[256];
	char pw[256];
	char topic[1024];
	FILE *fd;
	char pub_string[2048];
	char sub_string[2048];
	char reply_string[256];
	int lseqno;
	int code;
	int s;
	int err;
	TXL *tl;

	err = WooFLocalIP(gateway_ip,sizeof(gateway_ip));
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
	 * set up subscription
	 */
	memset(sub_string,0,sizeof(sub_string));
	sprintf(sub_string,"/usr/bin/mosquitto_sub -C 1 -h %s -t %s.input -u \'%s\' -P \'%s\'",
			gateway_ip,
			topic,
			user,
			pw);
	fd = popen(sub_string,"r");
	if(fd == NULL) {
		fprintf(stderr,"WooFGetLatestSeqnoMQTT: mosquitto_sub failed\n");
		return(-1);
	}

	/*
	 * make the request
	 */
	sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d\'",
		gateway_ip,
		topic,
		user,
		pw,
		wf_name,
		WOOF_MQTT_GET_LATEST_SEQNO);
	system(pub_string);

	/*
	 * now wait for the reply
	 * FIXME: should use select() so that it can time out the read
	 */
	memset(reply_string,0,sizeof(reply_string));
	s = read(fileno(fd),reply_string,sizeof(reply_string));
	pclose(fd);
	if(s > 0) {
		/*
		 * the topic, here, is really the woof_name -- used it to save space
		 */
		tl = ParseLine(reply_string,"|");
		if((tl == NULL) || (tl->list == NULL) ||
		(tl->list->first->next == NULL) || (tl->list->first->next->next == NULL)) {
			fprintf(stderr,
				"WooFGetLatestSeqnoMQTT: bad reply: %s\n",reply_string);
			if(tl != NULL) {
				DestroyTXL(tl);
			}
			return(-1);
		}
		code = atoi(tl->list->first->next->value.s);
		if(code != WOOF_MQTT_GET_LATEST_SEQNO_RESP) {
			fprintf(stderr,"WooFGetLatestSeqnoMQTT: bad resp code %d (should be %d)\n",
					code,
					WOOF_MQTT_GET_LATEST_SEQNO_RESP);
			DestroyTXL(tl);
			return(-1);
		}
		lseqno = atoi(tl->list->first->next->next->value.s);
		DestroyTXL(tl);
		return((unsigned long)lseqno);
	}
	return(-1);
}

unsigned long WooFPutMQTT(const char *wf_name, const char *wf_handler, const void* element)
{
	char gateway_ip[IPLEN];
	char topic[1024];
	char user[256];
	char pw[256];
	FILE *fd;
	unsigned long size;
	int err;
	char *payload;
	int count;
	char pub_string[1024*17];
	char sub_string[256];
	char reply_string[256];
	int seqno;
	int code;
	int s;
	TXL *tl;
	

	/*
	 * get the gateway IP
	 */
	err = WooFLocalIP(gateway_ip,sizeof(gateway_ip));
	if(err < 0) {
		return(-1);
	}
	
	size = WooFGetElSizeMQTT(wf_name);
	if(size == (unsigned long)-1) {
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
		fprintf(stderr,"WooFPutMQTT: mosquitto_sub failed\n");
		return(-1);
	}

	/*
	 * now do the send side
	 */

	/*
	 * get twice as many bytes for ascii encoding
	 */
	payload = (char *)malloc((size * 2)+1);
	memset(payload,0,(size*2)+1);
	if(payload == 0) {
		return(-1);
	}
	count = ConvertBinarytoASCII(payload,element,size);
	if(count < 0) {
		free(payload);
		return(-1);
	}

	memset(pub_string,0,sizeof(pub_string));
	if(wf_handler == NULL) {
		sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d|%s|%s\'",
			gateway_ip,
			topic,
			user,
			pw,
			wf_name,
			WOOF_MQTT_PUT,
			"NULL",
			payload);
	} else {
		sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d|%s|%s\'",
			gateway_ip,
			topic,
			user,
			pw,
			wf_name,
			WOOF_MQTT_PUT,
			wf_handler,
			payload);
	}
	free(payload);

	/*
	 * publish the put
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
		   (tl->list->first->next == NULL) || (tl->list->first->next->next == NULL)) {
			fprintf(stderr,
				"WooFPutMQTT: bad reply: %s\n",reply_string);
			if(tl != NULL) {
				DestroyTXL(tl);
			}
			return(-1);
		}
		code = atoi(tl->list->first->next->value.s);
		if(code != WOOF_MQTT_PUT_RESP) {
			fprintf(stderr,"WooFPutMQTT: bad resp code %d (should be %d)\n",
					code,
					WOOF_MQTT_PUT_RESP);
			DestroyTXL(tl);
			return(-1);
		}
		seqno = atoi(tl->list->first->next->next->value.s);
		DestroyTXL(tl);
		return((unsigned long)seqno);
	}
	return(-1);
}

int WooFGetMQTT(const char *wf_name, const void* element, const unsigned long seqno)
{
	char gateway_ip[IPLEN];
	char topic[1024];
	char user[256];
	char pw[256];
	FILE *fd;
	int err;
	char pub_string[1024*17];
	char sub_string[256];
	char reply_string[256];
	int code;
	int s;
	TXL *tl;
	int lsize;
	

	/*
	 * get the gateway IP
	 */
	err = WooFLocalIP(gateway_ip,sizeof(gateway_ip));
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
	sprintf(pub_string,"/usr/bin/mosquitto_pub -h %s -t %s.output -u \'%s\' -P \'%s\' -m \'%s|%d|%d\'",
			gateway_ip,
			topic,
			user,
			pw,
			wf_name,
			WOOF_MQTT_GET,
			seqno);
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

/*
 * API code starts here
 * 
 * note that some internal CSPOT calls are supported for handler compatibilty.
 */

/*
 * this function loads up the global variables with data needed to communicate
 * with the MQTT broker
 * 
 * note that these can also come from following environment variables
 * WOOFC_IP: gateway IP
 * WOOFC_DEVICE_NAME: name of this device
 * WOOFC_MQTT_USER: user name for MQTT broker
 * WOOFC_MQTT_PW: password for MQTT broker
 *
 */
void WooFDeviceInit(char* gateway_ip, 
		    char* device_name, int len,
		    char *user, int ulen,
		    char *pw, int pwlen)
{
	int llen;

	if(len < sizeof(Device_name)) {
		llen = len;
	} else {
		llen = sizeof(Device_name);
	}
	strncpy(Host_ip,gateway_ip,IPLEN);
	strncpy(Device_name,device_name,llen);

	if(ulen < sizeof(User_name)) {
		llen = ulen;
	} else {
		llen = sizeof(User_name);
	}
	strncpy(User_name,user,llen);

	if(pwlen < sizeof(Password)) {
		llen = pwlen;
	} else {
		llen = sizeof(Password);
	}
	strncpy(Password,pw,llen);

	return;
}

/*
 * external API calls
 */

unsigned long WooFPut(const char* wf_name, const char* wf_handler, const void* element)
{
	unsigned long seqno;
	int err;
	char local_name[WOOFDEV_MAX_NAME_SIZE];

	if(IsRemoteWooF(wf_name) == 1) {
		seqno = WooFPutMQTT(wf_name, wf_handler, element);
		return(seqno);
	}

	err = WooFNameFromURI(wf_name,local_name,sizeof(local_name));
	if(err < 0) {
		return((unsigned long)-1);
	}

	seqno = WooFDevPut(local_name,element);

	if(wf_handler != NULL) {
		fprintf(stderr,"WooFPut: handlers not implemented yet\n");
	}

	return (seqno);
}

unsigned long WooFGetLatestSeqno(const char *wf_name)
{
	unsigned long seqno;
	int err;
	char local_name[WOOFDEV_MAX_NAME_SIZE];

	if(IsRemoteWooF(wf_name) == 1) {
		seqno = WooFGetLatestSeqnoMQTT(wf_name);
		return(seqno);
	}

	err = WooFNameFromURI(wf_name,local_name,sizeof(local_name));
	if(err < 0) {
		return(-1);
	}
	seqno = WooFDevGetLatestSeqno(local_name);
	return (seqno);
}

int WooFGet(const char* wf_name, const void* element, const unsigned long seqno)
{
	int size;
	int err;
	char local_name[WOOFDEV_MAX_NAME_SIZE];

	if(IsRemoteWooF(wf_name) == 1) {
		err = WooFGetMQTT(wf_name, element, seqno);
		return(err);
	}

	err = WooFNameFromURI(wf_name,local_name,sizeof(local_name));
	if(err < 0) {
		return ((unsigned long)-1);
	}
	err = WooFDevGet(local_name,element,seqno);

	return(err);
}

int WooFCreate(const char* wf_name, unsigned long element_size, unsigned long history_size)
{
	int err;
	char local_name[WOOFDEV_MAX_NAME_SIZE]; /* these are small */


	err = WooFNameFromURI(wf_name,local_name,sizeof(local_name));
	if(err < 0) {
		return(-1);
	}
	/*
	 * WooFDevs are local only
	 */
	err = WooFDevCreate(wf_name,(int)element_size, (int)history_size);

	if(err < 0) {
		return(-1);
	}
	return(1);
}

/*
 * CSPOT internal calls (for compatibity with CSPOT handlers that use the fast path
 */

/*
 * internal CSPOT compatibility routines
 */

/*
 * create in-memory structure for underlying WOOFDev
 */
#ifdef NOTRIGHTNOW
WOOF *WooFOpen(const char* wf_name)
{
        WOOFDev *wd;
        int fd;
        int s;

        wd = malloc(sizeof(WOOFDev));
        if(wd == NULL) {
                return(NULL);
        }

        fd = open(wf_name, O_RDWR,0600);
        if(fd < 0) {
                free(wd);
                return(NULL);
        }

        s = read(fd,wd,sizeof(wd));
        if(s != sizeof(WOOFDev)) {
                free(wd);
                return(NULL);
        }
        close(fd);
        return((WOOF *)wd);
}


char *WoofGetFileName(WOOF *wf)
{
	if(wf == NULL) {
		return(NULL);
	}
	return(wf->wname);
}
	
void WooFDrop(WOOF *wf)
{
	if(wf == NULL) {
		return;
	}
	free(wf);
	return;
}

/*
 * weird routine that reverses elements in a buffer starting with the tail
 */
int WooFReadTail(WOOF *wf, void *elements, int element_count)
{
	int fd;
	int i;
	int err;
	int s;
	int curr_seqno;

	/*
	 * refresh the in-memory WOOFDev structure
	 * to get the tail
	 */
        fd = open(wf->wname,O_RDWR,0600);
        if(fd < 0) {
                free(wd);
                return(0);
        }

        s = read(fd,(WOOFDev *)wf,sizeof(WOOFDev));
        if(s != sizeof(WOOFDev)) {
		close(fd);
                return(0);
        }
	close(fd);

	curr_seqno = wf->seqno;
	for(i=0; i < element_count; i++) {
		err = WooFDevGet(wf->wname,&(elements[i]),curr_seqno);
		if(err < 0) {
			return(i);
		}
		curr_seqno--;
	}
	return(i);
}

#endif
