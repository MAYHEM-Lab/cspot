#define DEBUG
#define REPAIR

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

#include "uriparser2/uriparser2.h"

#include "woofc.h" /* for WooFPut */
#include "woofc-cache.h"
#include "woofc-access.h"
#include "dlist.h"

extern char WooF_namespace[2048];
extern char Host_ip[25];
extern unsigned long Name_id;

#ifdef REPAIR
extern LOG *Name_log;
#endif

WOOF_CACHE *WooF_cache;

/*
 * from https://en.wikipedia.org/wiki/Universal_hashing
 */
unsigned int WooFPortHash(char *namespace)
{
	unsigned long h = 5381;
	unsigned long a = 33;
	unsigned long i;

	for (i = 0; i < strlen(namespace); i++)
	{
		h = ((h * a) + namespace[i]); /* no mod p due to wrap */
	}

	/*
	 * hash namespace to port number between 50000 and 60000
	 */
	return (50000 + (h % 10000));
}

int WooFValidURI(char *str)
{
	char *prefix;
	/*
	 * must begin with woof://
	 */
	prefix = strstr(str, "woof://");
	if (prefix == str)
	{
		return (1);
	}
	else
	{
		return (0);
	}
}

/*
 * convert URI to namespace path
 */
int WooFURINameSpace(char *woof_uri_str, char *woof_namespace, int len)
{
	struct URI *uri;
	int i;

	if (!WooFValidURI(woof_uri_str))
	{ /* still might be local name, but return error */
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if (uri == NULL)
	{
		return (-1);
	}

	if (uri->path == NULL)
	{
		free(uri);
		return (-1);
	}
	strncpy(woof_namespace, uri->path, len);

	free(uri);

	return (1);
}

/*
 * extract namespace from full woof_name
 */
int WooFNameSpaceFromURI(char *woof_uri_str, char *woof_namespace, int len)
{
	struct URI *uri;
	int i;

	if (!WooFValidURI(woof_uri_str))
	{ /* still might be local name, but return error */
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if (uri == NULL)
	{
		return (-1);
	}

	if (uri->path == NULL)
	{
		free(uri);
		return (-1);
	}
	/*
	 * walk back to the last '/' character
	 */
	i = strlen(uri->path);
	while (i >= 0)
	{
		if (uri->path[i] == '/')
		{
			if (i <= 0)
			{
				free(uri);
				return (-1);
			}
			if (i > len)
			{ /* not enough space to hold path */
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

int WooFNameFromURI(char *woof_uri_str, char *woof_name, int len)
{
	struct URI *uri;
	int i;
	int j;

	if (!WooFValidURI(woof_uri_str))
	{
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if (uri == NULL)
	{
		return (-1);
	}

	if (uri->path == NULL)
	{
		free(uri);
		return (-1);
	}
	/*
	 * walk back to the last '/' character
	 */
	i = strlen(uri->path);
	j = 0;
	/*
	 * if last character in the path is a '/' this is an error
	 */
	if (uri->path[i] == '/')
	{
		free(uri);
		return (-1);
	}
	while (i >= 0)
	{
		if (uri->path[i] == '/')
		{
			i++;
			if (i <= 0)
			{
				free(uri);
				return (-1);
			}
			if (j > len)
			{ /* not enough space to hold path */
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
int WooFIPAddrFromURI(char *woof_uri_str, char *woof_ip, int len)
{
	struct URI *uri;
	int i;
	int j;
	struct in_addr addr;
	struct in_addr **list;
	struct hostent *he;
	int err;

	if (!WooFValidURI(woof_uri_str))
	{
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if (uri == NULL)
	{
		return (-1);
	}

	if (uri->host == NULL)
	{
		free(uri);
		return (-1);
	}

	/*
	 * test to see if the host is an IP address already
	 */
	err = inet_aton(uri->host, &addr);
	if (err == 1)
	{
		strncpy(woof_ip, uri->host, len);
		free(uri);
		return (1);
	}

	/*
	 * here, assume it is a DNS name
	 */
	he = gethostbyname(uri->host);
	if (he == NULL)
	{
		free(uri);
		return (-1);
	}

	list = (struct in_addr **)he->h_addr_list;
	if (list == NULL)
	{
		free(uri);
		return (-1);
	}

	for (i = 0; list[i] != NULL; i++)
	{
		/*
		 * return first non-NULL ip address
		 */
		if (list[i] != NULL)
		{
			strncpy(woof_ip, inet_ntoa(*list[i]), len);
			free(uri);
			return (1);
		}
	}

	free(uri);
	return (-1);
}

int WooFPortFromURI(char *woof_uri_str, int *woof_port)
{
	struct URI *uri;
	int err;

	if (!WooFValidURI(woof_uri_str))
	{
		return (-1);
	}

	uri = uri_parse(woof_uri_str);
	if (uri == NULL)
	{
		return (-1);
	}

	if (uri->port == 0)
	{
		free(uri);
		return (-1);
	}

	if (uri->port == -1)
	{
		free(uri);
		return (-1);
	}

	*woof_port = (int)uri->port;

	free(uri);
	return (1);
}

int WooFLocalIP(char *ip_str, int len)
{
	struct ifaddrs *addrs;
	struct ifaddrs *tmp;
	char *local_ip;

	/*
	 * check to see if we have been assigned a local Host_ip (say because we are in a container
	 */
	if (Host_ip[0] != 0)
	{
		strncpy(ip_str, Host_ip, len);
		return (1);
	}

	/*
	 * now check to see if there is a specific ip address we should use
	 * (e.g. this is a amulti-homed host and we want a specific NIC to be the NIC
	 * servicing requests
	 */
	local_ip = getenv("WOOFC_IP");
	if(local_ip != NULL) {
		strncpy(ip_str,local_ip,len);
		return(1);
	}
	
	

	/*
	 * need the non loop back IP address for local machine to allow inter-container
	 * communication (e.g. localhost won't work)
	 */
	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
		{
			if (strcmp(tmp->ifa_name, "lo") != 0)
			{
				struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
				strncpy(ip_str, inet_ntoa(pAddr->sin_addr), len);
				freeifaddrs(addrs);
				return (1);
			}
		}
		tmp = tmp->ifa_next;
	}
	fprintf(stderr, "WooFLocalIP: no local IP address found\n");
	fflush(stderr);
	freeifaddrs(addrs);
	exit(1);
}

int WooFLocalName(char *woof_name, char *local_name, int len)
{
	return (WooFNameFromURI(woof_name, local_name, len));
}

static zmsg_t *ServerRequest(char *endpoint, zmsg_t *msg)
{
	zsock_t *server;
	zpoller_t *resp_poll;
	zsock_t *server_resp;
	int err;
	zmsg_t *r_msg;

	/*
	 * get a socket to the server
	 */
	server = zsock_new_req(endpoint);
	if (server == NULL)
	{
		fprintf(stderr, "ServerRequest: no server connection to %s\n",
				endpoint);
		fflush(stderr);
		zmsg_destroy(&msg);
		return (NULL);
	}

	/*
	 * set up the poller for the reply
	 */
	resp_poll = zpoller_new(server, NULL);
	if (resp_poll == NULL)
	{
		fprintf(stderr, "ServerRequest: no poller for reply from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zmsg_destroy(&msg);
		return (NULL);
	}

	/*
	 * send the message to the server
	 */
	err = zmsg_send(&msg, server);
	if (err < 0)
	{
		fprintf(stderr, "ServerRequest: msg send to %s failed\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		zmsg_destroy(&msg);
		return (NULL);
	}

	/*
	 * wait for the reply, but not indefinitely
	 */
	server_resp = zpoller_wait(resp_poll, WOOF_MSG_REQ_TIMEOUT);
	if (server_resp != NULL)
	{
		r_msg = zmsg_recv(server_resp);
		if (r_msg == NULL)
		{
			fprintf(stderr, "ServerRequest: msg recv from %s failed\n",
					endpoint);
			fflush(stderr);
			zsock_destroy(&server);
			zpoller_destroy(&resp_poll);
			return (NULL);
		}
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (r_msg);
	}
	if (zpoller_expired(resp_poll))
	{
		fprintf(stderr, "ServerRequest: msg recv timeout from %s after %d msec\n",
				endpoint, WOOF_MSG_REQ_TIMEOUT);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (NULL);
	}
	else if (zpoller_terminated(resp_poll))
	{
		fprintf(stderr, "ServerRequest: msg recv interrupted from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (NULL);
	}
	else
	{
		fprintf(stderr, "ServerRequest: msg recv failed from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (NULL);
	}
}

void WooFProcessPut(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	char hand_name[2048];
	char local_name[2048];
	unsigned int copy_size;
	void *element;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;
	unsigned long cause_host;
	unsigned long cause_seq_no;

#ifdef DEBUG
	printf("WooProcessPut: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFPut requires a woof_name, handler_name, and the data
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessPut: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessPut: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

#ifdef DEBUG
	printf("WooFProcessPut: received woof_name %s\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * handler name in the second frame
	 */
	memset(hand_name, 0, sizeof(hand_name));
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	/*
	 * could be zero if there is no handler
	 */
	if (copy_size > 0)
	{
		str = (char *)zframe_data(frame);
		if (copy_size > (sizeof(hand_name) - 1))
		{
			copy_size = sizeof(hand_name) - 1;
		}
		strncpy(hand_name, str, copy_size);
	}

#ifdef DEBUG
	printf("WooFProcessPut: received handler name %s\n", hand_name);
	fflush(stdout);
#endif

	/*
	 * cause_host in the third frame
	 */
	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessPut: couldn't find cause_host in msg");
		return;
	}
	copy_size = zframe_size(frame);
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, zframe_data(frame), copy_size);
	cause_host = strtoul(buffer, (char **)NULL, 10);
#ifdef DEBUG
	printf("WooFProcessPut: received cause_host %lu\n", cause_host);
	fflush(stdout);
#endif

	/*
	 * cause_seq_no in the fourth frame
	 */
	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessPut: couldn't find cause_seq_no in msg");
		return;
	}
	copy_size = zframe_size(frame);
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, zframe_data(frame), copy_size);
	cause_seq_no = strtoul(buffer, (char **)NULL, 10);

#ifdef DEBUG
	printf("WooFProcessPut: received cause_seq_no %lu\n", cause_seq_no);
	fflush(stdout);
#endif

	/*
	 * raw data in the fifth frame
	 *
	 * need a maximum size but for now see if we can malloc() the space
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	element = malloc(copy_size);
	if (element == NULL)
	{ /* element too big */
		fprintf(stderr, "WooFMsgProcessPut: woof_name: %s, handler: %s, size %lu too big\n",
				woof_name, hand_name, copy_size);
		fflush(stderr);
		seq_no = -1;
	}
	else
	{
		str = (char *)zframe_data(frame);
		memcpy(element, (void *)str, copy_size);
#ifdef DEBUG
		printf("WooFProcessPut: received %lu bytes for the element\n", copy_size);
		fflush(stdout);
#endif

		/*
		 * FIX ME: in a cloud, the calling process doesn't know the publically viewable
		 * IP address so it can't determine whether the access is local or not.
		 *
		 * For now, assume that all Process functions convert to a local access
		 */
		memset(local_name, 0, sizeof(local_name));
		err = WooFLocalName(woof_name, local_name, sizeof(local_name));
		if (err < 0)
		{
			strncpy(local_name, woof_name, sizeof(local_name));
		}

		/*
		 * attempt to put the element into the local woof_name
		 *
		 * note that we could sanity check the woof_name against the local woof_name
		 * but WooFPut does this also
		 */
		if (hand_name[0] != 0)
		{
			seq_no = WooFPutWithCause(local_name, hand_name, element, cause_host, cause_seq_no);
		}
		else
		{
			seq_no = WooFPutWithCause(local_name, NULL, element, cause_host, cause_seq_no);
		}

		free(element);
	}

	/*
	 * send seq_no back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessPut: no reply message");
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", seq_no);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessPut: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}

	return;
}

void WooFProcessGetElSize(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	char local_name[2048];
	unsigned int copy_size;
	void *element;
	unsigned el_size;
	char buffer[255];
	int err;
	WOOF *wf;

#ifdef DEBUG
	printf("WooFProcessGetElSize: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFElSize requires a woof_name
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetElSize: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetElSize: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

	/*
	 * FIX ME: for now, make all Process requests local
	 */
	memset(local_name, 0, sizeof(local_name));
	err = WooFLocalName(woof_name, local_name, sizeof(local_name));

	if (err < 0)
	{
		wf = WooFOpen(local_name);
	}
	else
	{
		wf = WooFOpen(woof_name);
	}

	if (wf == NULL)
	{
		fprintf(stderr, "WooFProcessGetElSize: couldn't open %s (%s)\n", local_name, woof_name);
		fflush(stderr);
		el_size = -1;
	}
	else
	{
		el_size = wf->shared->element_size;
		WooFFree(wf);
	}

#ifdef DEBUG
	printf("WooFProcessGetElSize: woof_name %s has element size: %lu\n", woof_name, el_size);
	fflush(stdout);
#endif

	/*
	 * send el_size back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGetElSize: no reply message");
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", el_size);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessGetElSize: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGetElSize: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}

	return;
}

void WooFProcessGetLatestSeqno(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	char local_name[2048];
	unsigned int copy_size;
	unsigned latest_seq_no;
	char buffer[255];
	int err;
	WOOF *wf;

#ifdef DEBUG
	printf("WooFProcessGetLatestSeqno: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFLatestSeqno requires a woof_name
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetLatestSeqno: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetLatestSeqno: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

	/*
	 * FIX ME: for now, all process requests are local
	 */
	memset(local_name, 0, sizeof(local_name));
	err = WooFLocalName(woof_name, local_name, sizeof(local_name));
	if (err < 0)
	{
		wf = WooFOpen(woof_name);
	}
	else
	{
		wf = WooFOpen(local_name);
	}
	if (wf == NULL)
	{
		fprintf(stderr, "WooFProcessGetLatestSeqno: couldn't open %s\n", woof_name);
		fflush(stderr);
		latest_seq_no = -1;
	}
	else
	{
		latest_seq_no = WooFLatestSeqno(wf);
		WooFFree(wf);
	}

#ifdef DEBUG
	printf("WooFProcessGetLatestSeqno: woof_name %s has latest seq_no: %lu\n", woof_name, latest_seq_no);
	fflush(stdout);
#endif

	/*
	 * send latest_seq_no back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGetLatestSeqno: no reply message");
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", latest_seq_no);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessGetLatestSeqno: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGetLatestSeqno: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}

	return;
}

unsigned long WooFMsgGetLatestSeqno(char *woof_name)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;
	unsigned long lastest_seq_no;
	int err;

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * try local addr
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetLatestSeqno: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a GetElSize message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_GET_LATEST_SEQNO);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s no frame for WOOF_MSG_GET_LATEST_SEQNO command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetLatestSeqno: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s got WOOF_MSG_GET_LATEST_SEQNO (%s) command frame frame\n", woof_name, buffer);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s can't append WOOF_MSG_GET_LATEST_SEQNO command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetLatestSeqno: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetLatestSeqno: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetLatestSeqno: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);

	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s couldn't recv msg for element size from server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetLatestSeqno: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGetLatestSeqno: woof: %s no recv frame for from server at %s\n",
					woof_name, endpoint);
			perror("WooFMsgGetLatestSeqno: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		str = zframe_data(r_frame);
		lastest_seq_no = strtoul(str, (char **)NULL, 10);
		zmsg_destroy(&r_msg);
	}

#ifdef DEBUG
	printf("WooFMsgGetLatestSeqno: woof: %s recvd size: %lu message from server at %s\n",
		   woof_name, lastest_seq_no, endpoint);
	fflush(stdout);

#endif

	return (lastest_seq_no);
}

void WooFProcessGetTail(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	char local_name[2048];
	unsigned int copy_size;
	void *element;
	unsigned el_size;
	char buffer[255];
	int err;
	WOOF *wf;
	unsigned long el_read;
	unsigned long el_count;
	void *ptr;

#ifdef DEBUG
	printf("WooFProcessGetTail: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFElSize requires a woof_name
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetTail: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetTail: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

	/*
	 * next frame is the number of elements
	 */
	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetTail: couldn't find element count in msg");
		return;
	}
	el_count = strtoul(zframe_data(frame), (char **)NULL, 10);

	/*
	 * FIX ME: for now, all process requests are local
	 */
	memset(local_name, 0, sizeof(local_name));
	err = WooFLocalName(woof_name, local_name, sizeof(local_name));

	if (err < 0)
	{
		wf = WooFOpen(woof_name);
	}
	else
	{
		wf = WooFOpen(local_name);
	}
	if (wf == NULL)
	{
		fprintf(stderr, "WooFProcessGetTail: couldn't open %s\n", woof_name);
		fflush(stderr);
		el_read = 0;
	}
	else
	{
		el_size = wf->shared->element_size;
		WooFFree(wf);
	}

	if (wf != NULL)
	{
		ptr = malloc(el_size * el_count);
		if (ptr == NULL)
		{
			fprintf(stderr, "WooFProcessGetTail: couldn't open %s\n", woof_name);
			fflush(stderr);
			WooFFree(wf);
			return;
		}

		el_read = WooFReadTail(wf, ptr, el_count);
		WooFFree(wf);
	}

#ifdef DEBUG
	printf("WooFProcessGetTail: woof_name %s read %lu elements\n", woof_name, el_read);
	fflush(stdout);
#endif

	/*
	 * send el_read and elements back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGetElSize: no reply message");
		if (ptr != NULL)
		{
			free(ptr);
		}
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", el_read);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessGetTail: no reply frame");
		zmsg_destroy(&r_msg);
		if (ptr != NULL)
		{
			free(ptr);
		}
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessGetTail: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		if (ptr != NULL)
		{
			free(ptr);
		}
		return;
	}

	r_frame = zframe_new(ptr, el_read * el_size);
	if (r_frame == NULL)
	{
		perror("WooFProcessGetTail: no second reply frame");
		zmsg_destroy(&r_msg);
		if (ptr != NULL)
		{
			free(ptr);
		}
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessGetTail: couldn't second append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		if (ptr != NULL)
		{
			free(ptr);
		}
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGetElSize: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		if (ptr != NULL)
		{
			free(ptr);
		}
		return;
	}

	free(ptr);

	return;
}

void WooFProcessGet(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;

	char woof_name[2048];
	char hand_name[2048];
	char local_name[2048];
	unsigned int copy_size;
	void *element;
	unsigned long el_size;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;
	WOOF *wf;
	unsigned long cause_host;
	unsigned long cause_seq_no;

#ifdef DEBUG
	printf("WooFProcessGet: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFGet requires a woof_name, and the seq_no
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGet: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGet: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

#ifdef DEBUG
	printf("WooFProcessGet: received woof_name %s\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * seq_no name in the second frame
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	if (copy_size > 0)
	{
		seq_no = strtoul(zframe_data(frame), (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFProcessGet: received seq_no name %lu\n", seq_no);
		fflush(stdout);
#endif

		/*
		 * cause_host in the third frame
		 */
		frame = zmsg_next(req_msg);
		if (frame == NULL)
		{
			perror("WooFProcessGet: couldn't find cause_host in msg");
			return;
		}
		copy_size = zframe_size(frame);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, zframe_data(frame), copy_size);
		cause_host = strtoul(buffer, (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFProcessGet: received cause_host %lu\n", cause_host);
		fflush(stdout);
#endif

		/*
		 * cause_seq_no in the fourth frame
		 */
		frame = zmsg_next(req_msg);
		copy_size = zframe_size(frame);
		if (frame == NULL)
		{
			perror("WooFProcessGet: couldn't find cause_seq_no in msg");
			return;
		}
		copy_size = zframe_size(frame);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, zframe_data(frame), copy_size);
		cause_seq_no = strtoul(buffer, (char **)NULL, 10);

		/*
		 * FIX ME: for now, all process requests are local
		 */
		memset(local_name, 0, sizeof(local_name));
		err = WooFLocalName(woof_name, local_name, sizeof(local_name));
		/*
		 * attempt to get the element from the local woof_name
		 */
		if (err < 0)
		{
			wf = WooFOpen(woof_name);
		}
		else
		{
			wf = WooFOpen(local_name);
		}
		if (wf == NULL)
		{
			fprintf(stderr, "WooFProcessGet: couldn't open woof: %s\n",
					woof_name);
			fflush(stderr);
			element = NULL;
			el_size = 0;
		}
		else
		{
			el_size = wf->shared->element_size;
			element = malloc(el_size);
			if (element == NULL)
			{
				fprintf(stderr, "WooFProcessGet: no space woof: %s\n",
						woof_name);
				fflush(stderr);
			}
			else
			{
				err = WooFReadWithCause(wf, element, seq_no, cause_host, cause_seq_no);
				if (err < 0)
				{
					fprintf(stderr,
							"WooFProcessGet: read failed: %s at %lu\n",
							woof_name, seq_no);
					free(element);
					element = NULL;
					el_size = 0;
				}
			}
			WooFFree(wf);
		}
	}
	else
	{ /* copy_size <= 0 */
		fprintf(stderr, "WooFProcessGet: no seq_no frame data for %s\n",
				woof_name);
		fflush(stderr);
		element = NULL;
		el_size = 0;
	}

	/*
	 * send element back or empty frame indicating no dice
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGet: no reply message");
		if (element != NULL)
		{
			free(element);
		}
		return;
	}
	if (element != NULL)
	{
#ifdef DEBUG
		printf("WooFProcessGet: creating frame for %lu bytes of element at seq_no %lu\n",
			   el_size, seq_no);
		fflush(stdout);
#endif

		r_frame = zframe_new(element, el_size);
		if (r_frame == NULL)
		{
			perror("WooFProcessGet: no reply frame");
			free(element);
			zmsg_destroy(&r_msg);
			return;
		}
	}
	else
	{
#ifdef DEBUG
		printf("WooFProcessGet: creating empty frame\n");
		fflush(stdout);
#endif
		r_frame = zframe_new_empty();
		if (r_frame == NULL)
		{
			perror("WooFProcessGet: no empty reply frame");
			zmsg_destroy(&r_msg);
			return;
		}
	}
	err = zmsg_append(r_msg, &r_frame);
	if (element != NULL)
	{
		free(element);
	}
	if (err != 0)
	{
		perror("WooFProcessGet: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGet: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}

	return;
}

#ifdef DONEFLAG
void WooFProcessGetDone(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;

	char woof_name[2048];
	char hand_name[2048];
	char local_name[2048];
	unsigned int copy_size;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;
	WOOF *wf;
	unsigned long done;

#ifdef DEBUG
	printf("WooFProcessGetDone: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFGetDone requires a woof_name, and the seq_no
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetDone: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetDone: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

#ifdef DEBUG
	printf("WooFProcessGetDone: received woof_name %s\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * seq_no name in the second frame
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	if (copy_size > 0)
	{
		seq_no = strtoul(zframe_data(frame), (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFProcessGetDone: received seq_no name %lu\n", seq_no);
		fflush(stdout);
#endif
		/*
		 * FIX ME: for now, all process requests are local
		 */
		memset(local_name, 0, sizeof(local_name));
		err = WooFLocalName(woof_name, local_name, sizeof(local_name));
		/*
		 * attempt to get the element from the local woof_name
		 */
		if (err < 0)
		{
			wf = WooFOpen(woof_name);
		}
		else
		{
			wf = WooFOpen(local_name);
		}
		if (wf == NULL)
		{
			fprintf(stderr, "WooFProcessGetDone: couldn't open woof: %s\n",
					woof_name);
			fflush(stderr);
			done = -1;
		}
		else
		{
			done = wf->shared->done;
			WooFFree(wf);
		}
	}
	else
	{ /* copy_size <= 0 */
		fprintf(stderr, "WooFProcessGetDone: no seq_no frame data for %s\n",
				woof_name);
		fflush(stderr);
		done = -1;
	}

	/*
	 * send element back or empty frame indicating no dice
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGetDone: no reply message");
		return;
	}
#ifdef DEBUG
	printf("WooFProcessGetDone: creating frame for %lu bytes of element at seq_no %lu\n",
		   sizeof(done), seq_no);
	fflush(stdout);
#endif

	r_frame = zframe_new(&done, sizeof(done));
	if (r_frame == NULL)
	{
		perror("WooFProcessGetDone: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessGetDone: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGetDone: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}

	return;
}

#endif

#ifdef REPAIR
void LogProcessGetSize(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	unsigned int copy_size;
	void *log_block;
	char buffer[255];
	int err;
	unsigned long int space;

#ifdef DEBUG
	printf("LogProcessGetSize: called\n");
	fflush(stdout);
#endif

	/*
	 * LogProcessGetSize has no parameter frame, go straight to reply message.
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("LogProcessGetSize: no reply message");
		return;
	}

	log_block = (void *)Name_log;
	space = Name_log->size * sizeof(EVENT) + sizeof(LOG);
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", space);
#ifdef DEBUG
	printf("LogProcessGetSize: creating frame for log size\n");
	fflush(stdout);
#endif
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("LogProcessGetSize: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}

	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("LogProcessGetSize: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("LogProcessGetSize: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}
	return;
}

void LogProcessGet(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	unsigned int copy_size;
	void *log_block;
	char buffer[255];
	int err;
	unsigned long int space;

#ifdef DEBUG
	printf("LogProcessGet: called\n");
	fflush(stdout);
#endif

	/*
	 * LogProcessGet has no parameter frame, go straight to reply message.
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("LogProcessGet: no reply message");
		return;
	}

	log_block = (void *)Name_log;
	space = Name_log->size * sizeof(EVENT) + sizeof(LOG);

#ifdef DEBUG
	printf("LogProcessGet: creating frame for %lu bytes of log_block\n",
		   space);
	fflush(stdout);
#endif
	r_frame = zframe_new(log_block, space);
	if (r_frame == NULL)
	{
		perror("LogProcessGet: no reply frame");
		free(log_block);
		zmsg_destroy(&r_msg);
		return;
	}

	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("LogProcessGet: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("LogProcessGet: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}
	log_block = NULL;
	return;
}

/* TODO:
 */
void WooFProcessRepair(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char woof_name[2048];
	char *str;
	unsigned int copy_size;
	unsigned long count;
	unsigned long *seq_no;
	int err;
	char buffer[255];

#ifdef DEBUG
	printf("WooFProcessRepair: called\n");
	fflush(stdout);
#endif

	/*
	 * WooFProcessRepair requires a woof_name, count, and the seq_no
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessRepair: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessRepair: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

#ifdef DEBUG
	printf("WooFProcessRepair: received woof_name %s\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * count in the second frame
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	if (copy_size > 0)
	{
		count = strtoul(zframe_data(frame), (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFProcessRepair: received count %lu\n", count);
		fflush(stdout);
#endif
	}
	else
	{ /* copy_size <= 0 */
		fprintf(stderr, "WooFProcessRepair: no count frame data for %s\n", woof_name);
		fflush(stderr);
	}

	/*
	 * seq_no in the third frame
	 */
	seq_no = malloc(count * sizeof(unsigned long));
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	if (copy_size > 0)
	{
		memcpy(seq_no, zframe_data(frame), copy_size);
#ifdef DEBUG
		printf("WooFProcessRepair: received %lu seq_nos with size %lu\n", count, copy_size);
		fflush(stdout);
#endif
	}
	else
	{ /* copy_size <= 0 */
		fprintf(stderr, "WooFProcessRepair: no seq_no frame data for %s\n",
				woof_name);
		fflush(stderr);
	}

	/*
	 * send count back for confirmation
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessRepair: no reply message");
		free(seq_no);
		return;
	}
#ifdef DEBUG
	printf("WooFProcessRepair: creating frame for count %lu\n", count);
	fflush(stdout);
#endif

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", count);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessRepair: no reply frame");
		zmsg_destroy(&r_msg);
		free(seq_no);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessRepair: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		free(seq_no);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessRepair: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		free(seq_no);
		return;
	}
	// TODO: start repair here

	free(seq_no);
	return;
}
#endif

void *WooFMsgThread(void *arg)
{
	zsock_t *receiver;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	unsigned long tag;
	char *str;

	int err;

	/*
	 * right now, we use REQ-REP pattern from ZeroMQ.  need a way to timeout, however, as this
	 * pattern blocks indefinitely on network partition
	 */

	/*
	 * create a reply zsock and connect it to the back end of the proxy in the msg server
	 */
	receiver = zsock_new_rep(">inproc://workers");
	if (receiver == NULL)
	{
		perror("WooFMsgThread: couldn't open receiver");
		pthread_exit(NULL);
	}

#ifdef DEBUG
	printf("WooFMsgThread: about to call receive\n");
	fflush(stdout);
#endif
	msg = zmsg_recv(receiver);
	while (msg != NULL)
	{
#ifdef DEBUG
		printf("WooFMsgThread: received\n");
		fflush(stdout);
#endif
		/*
		 * WooFMsg starts with a message tag for dispatch
		 */
		frame = zmsg_first(msg);
		if (frame == NULL)
		{
			perror("WooFMsgThread: couldn't get tag");
			exit(1);
		}
		/*
		 * tag in the first frame
		 */
		str = (char *)zframe_data(frame);
		str[1] = 0;
		tag = strtoul(str, (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFMsgThread: processing msg with tag: %d\n", tag);
		fflush(stdout);
#endif

		switch (tag)
		{
		case WOOF_MSG_PUT:
			WooFProcessPut(msg, receiver);
			break;
		case WOOF_MSG_GET:
			WooFProcessGet(msg, receiver);
			break;
		case WOOF_MSG_GET_EL_SIZE:
			WooFProcessGetElSize(msg, receiver);
			break;
		case WOOF_MSG_GET_TAIL:
			WooFProcessGetTail(msg, receiver);
			break;
		case WOOF_MSG_GET_LATEST_SEQNO:
			WooFProcessGetLatestSeqno(msg, receiver);
			break;
#ifdef DONEFLAG
		case WOOF_MSG_GET_DONE:
			WooFProcessGetDone(msg, receiver);
			break;
#endif
#ifdef REPAIR
		case LOG_GET_REMOTE_SIZE:
			LogProcessGetSize(msg, receiver);
			break;
		case LOG_GET_REMOTE:
			LogProcessGet(msg, receiver);
			break;
		case WOOF_MSG_REPAIR:
			WooFProcessRepair(msg, receiver);
			break;
#endif
		default:
			fprintf(stderr, "WooFMsgThread: unknown tag %s\n",
					str);
			break;
		}
		/*
		 * process routines do not destroy request msg
		 */
		zmsg_destroy(&msg);

		/*
		 * wait for next request
		 */
		msg = zmsg_recv(receiver);
	}

	zsock_destroy(&receiver);
	pthread_exit(NULL);
}

int WooFMsgServer(char *namespace)
{

	int port;
	zactor_t *proxy;
	int err;
	char endpoint[255];
	pthread_t tids[WOOF_MSG_THREADS];
	int i;

	zsock_t *frontend;
	zsock_t *workers;
	zmsg_t *msg;

	if (namespace[0] == 0)
	{
		fprintf(stderr, "WooFMsgServer: couldn't find namespace\n");
		fflush(stderr);
		exit(1);
	}

#ifdef DEBUG
	printf("WooFMsgServer: started for namespace %s\n", namespace);
	fflush(stdout);
#endif

	/*
	 * set up the front end router socket
	 */
	memset(endpoint, 0, sizeof(endpoint));

	port = WooFPortHash(namespace);

	/*
	 * listen to any tcp address on port hash of namespace
	 */
	sprintf(endpoint, "tcp://*:%d", port);

#ifdef DEBUG
	printf("WooFMsgServer: fontend at %s\n", endpoint);
	fflush(stdout);
#endif

	/*
	 * create zproxy actor
	 */
	proxy = zactor_new(zproxy, NULL);
	if (proxy == NULL)
	{
		perror("WooFMsgServer: couldn't create proxy");
		exit(1);
	}

	/*
	 * create and bind endpoint with port has to frontend zsock
	 */
	zstr_sendx(proxy, "FRONTEND", "ROUTER", endpoint, NULL);
	zsock_wait(proxy);

	/*
	 * inproc:// is a process internal enpoint for ZeroMQ
	 *
	 * if backend isn't in this process, this endpoint will need to be
	 * some kind of IPC endpoit.  For now, assume it is within the process
	 * and handled by threads
	 */
	zstr_sendx(proxy, "BACKEND", "DEALER", "inproc://workers", NULL);
	zsock_wait(proxy);

	/*
	 * create a single thread for now.  The DEALER pattern can handle multiple threads, however
	 * so this can be increased if need be
	 */
	for (i = 0; i < WOOF_MSG_THREADS; i++)
	{
		err = pthread_create(&tids[i], NULL, WooFMsgThread, NULL);
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgServer: couldn't create thread %d\n", i);
			exit(1);
		}
	}

	/*
	 * right now, there is no way for these threads to exit so the msg server will block
	 * indefinitely in this join
	 */
	for (i = 0; i < WOOF_MSG_THREADS; i++)
	{
		pthread_join(tids[i], NULL);
	}

	/*
	 * we'll get here if the Msg thread is ever pthread_canceled()
	 */
	zactor_destroy(&proxy);

	exit(0);
}

unsigned long WooFMsgGetElSize(char *woof_name)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;

	unsigned long el_size;
	int err;
	unsigned long *el_size_cached;
	void *payload;

	if (WooF_cache == NULL)
	{
		WooF_cache = WooFCacheInit(WOOF_MSG_CACHE_SIZE);
		if (WooF_cache == NULL)
		{
			fprintf(stderr, "WooFMsgGetElSize: woof: %s cache init failed\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	el_size_cached = (unsigned long *)WooFCacheFind(WooF_cache, woof_name);
	if (el_size_cached != NULL)
	{
#ifdef DEBUG
		fprintf(stdout, "WooFMsgGetElSize: found %lu size for %s\n",
				*el_size_cached, woof_name);
		fflush(stdout);
#endif
		return (*el_size_cached);
	}

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * try local addr
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgGetElSize: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a GetElSize message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_GET_EL_SIZE);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s no frame for WOOF_MSG_GET_EL_SIZE command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s got WOOF_MSG_GET_EL_SIZE command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s can't append WOOF_MSG_GET_EL_SIZE command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);

	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetElSize: woof: %s couldn't recv msg for element size from server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGetElSize: woof: %s no recv frame for from server at %s\n",
					woof_name, endpoint);
			perror("WooFMsgGetElSize: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		str = zframe_data(r_frame);
		el_size = strtoul(str, (char **)NULL, 10);
		zmsg_destroy(&r_msg);
	}

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s recvd size: %lu message from server at %s\n",
		   woof_name, el_size, endpoint);
	fflush(stdout);

#endif

	el_size_cached = (unsigned long *)malloc(sizeof(unsigned long));
	if (el_size_cached != NULL)
	{
		*el_size_cached = el_size;
		err = WooFCacheInsert(WooF_cache, woof_name, (void *)el_size_cached);
		if (err < 0)
		{
			payload = WooFCacheAge(WooF_cache);
			if (payload != NULL)
			{
				free(payload);
			}
			err = WooFCacheInsert(WooF_cache, woof_name, (void *)el_size_cached);
			if (err < 0)
			{
				fprintf(stderr, "WooFMsgGetElSize: cache insert failed\n");
				fflush(stderr);
				free(el_size_cached);
			}
		}
	}

	return (el_size);
}

unsigned long WooFMsgGetTail(char *woof_name, void *elements, unsigned long el_size, int el_count)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;

	int err;
	unsigned long el_read;

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * try local addr
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgGetTail: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgGetElTail: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a GetTail message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_GET_TAIL);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s no frame for WOOF_MSG_GET_TAIL command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s got WOOF_MSG_GET_TAIL command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetTAIL: woof: %s can't append WOOF_MSG_GET_TAIL command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame with the element count
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", el_count);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s no frame for el_count to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s got el_count frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the el_count frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s can't append el_count to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetTail: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s added el_count to frame\n", woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);

	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetTail: woof: %s couldn't recv msg for element size from server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetElSize: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGetTail: woof: %s no recv frame for from server at %s\n",
					woof_name, endpoint);
			perror("WooFMsgGetTail: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		str = zframe_data(r_frame);
		el_read = strtoul(str, (char **)NULL, 10);

		r_frame = zmsg_next(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGetTail: woof: %s no second recv frame for from server at %s\n",
					woof_name, endpoint);
			perror("WooFMsgGetTail: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		if (el_read <= el_count)
		{
			memcpy(elements, zframe_data(r_frame), el_read * el_size);
		}
		else
		{
			memcpy(elements, zframe_data(r_frame), el_count * el_size);
		}
		zmsg_destroy(&r_msg);
	}

#ifdef DEBUG
	printf("WooFMsgGetTail: woof: %s recvd size: %lu message from server at %s\n",
		   woof_name, el_size, endpoint);
	fflush(stdout);

#endif

	if (el_read <= el_count)
	{
		return (el_read);
	}
	else
	{
		return (el_count);
	}
}

unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element, unsigned long el_size)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;

	struct timeval tm;
	unsigned long seq_no;
	int err;
	char *namelog_seq_no;
	unsigned long my_log_seq_no;

	if (el_size == (unsigned long)-1)
	{
		return (-1);
	}

	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if (namelog_seq_no != NULL)
	{
		my_log_seq_no = strtoul(namelog_seq_no, (char **)NULL, 10);
	}
	else
	{
		my_log_seq_no = 0;
	}

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * assume it is local
		 */
		memset(ip_str, 0, sizeof(ip_str));
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgPut: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a put message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_PUT);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no frame for WOOF_MSG_PUT command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got WOOF_MSG_PUT command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s can't append WOOF_MSG_PUT command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the handler name
	 * could be NULL
	 */

	if (hand_name != NULL)
	{
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, hand_name, sizeof(buffer));
		frame = zframe_new(buffer, strlen(buffer));
	}
	else
	{ /* czmq indicate this will work */
		frame = zframe_new_empty();
	}
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no frame for handler name %s to server at %s\n",
				woof_name, hand_name, endpoint);
		perror("WooFMsgPut: couldn't get new handler frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got frame for handler name %s\n", woof_name, hand_name);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s couldn't append frame for handler name %s to server at %s\n",
				woof_name, hand_name, endpoint);
		perror("WooFMsgPut: couldn't append handler frame");
		fflush(stderr);
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s appended frame for handler name %s\n", woof_name, hand_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s for new frame for 0x%x, size %lu\n",
		   woof_name, element, el_size);
	fflush(stdout);
#endif

	/*
	 * make a frame for the cause_host
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", Name_id);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no frame for cause_host to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got frame for cause_host %lu\n", woof_name, Name_id);
	fflush(stdout);
#endif

	/*
	 * add the cause_host frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't append cause_host frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s added cause_host to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the cause_seq_no
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", my_log_seq_no);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no frame for cause_seq_no to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got frame for cause_seq_no %lu\n", woof_name, my_log_seq_no);
	fflush(stdout);
#endif

	/*
	 * add the cause_seq_no frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgPut: couldn't append cause_seq_no frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s added cause_host to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
 	* make a frame for the element
 	*/
	frame = zframe_new(element, el_size);
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s no frame size %lu for handler name %s to server at %s\n",
				woof_name, el_size, hand_name, endpoint);
		perror("WooFMsgPut: couldn't get element frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got frame for element size %d\n", woof_name, el_size);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s couldn't append frame for element size %lu name %s to server at %s\n",
				woof_name, el_size, hand_name, endpoint);
		perror("WooFMsgPut: couldn't append element frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s appended frame for element size %d\n", woof_name, el_size);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgPut: woof: %s couldn't recv msg for element size %lu name %s to server at %s\n",
				woof_name, el_size, hand_name, endpoint);
		perror("WooFMsgPut: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgPut: woof: %s no recv frame for element size %lu name %s to server at %s\n",
					woof_name, el_size, hand_name, endpoint);
			perror("WooFMsgPut: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		str = zframe_data(r_frame);
		seq_no = strtoul(str, (char **)NULL, 10);
		zmsg_destroy(&r_msg);
	}

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s recvd seq_no %lu message to server at %s\n",
		   woof_name, seq_no, endpoint);
	fflush(stdout);

#endif
	return (seq_no);
}

int WooFMsgGet(char *woof_name, void *element, unsigned long el_size, unsigned long seq_no)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	int r_size;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;
	struct timeval tm;
	int err;
	char *namelog_seq_no;
	unsigned long my_log_seq_no;

	namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO");
	if (namelog_seq_no != NULL)
	{
		my_log_seq_no = strtoul(namelog_seq_no, (char **)NULL, 10);
	}
	else
	{
		my_log_seq_no = 0;
	}

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * assume it is local
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a put message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_GET);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for WOOF_MSG_GET command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got WOOF_MSG_GET command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s can't append WOOF_MSG_GET command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the seq_no
	 */

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", seq_no);
	frame = zframe_new(buffer, strlen(buffer));

	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for seq_no %lu server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGet: couldn't get new seq_no frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got frame for seq_no %lu\n", woof_name, seq_no);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s couldn't append frame for seq_no %lu to server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGet: couldn't append seq_no frame");
		fflush(stderr);
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s appended frame for seq_no %lu\n", woof_name, seq_no);
	fflush(stdout);
#endif

	/*
	 * make a frame for the cause_host
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", Name_id);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for cause_host to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got frame for cause_host %lu\n", woof_name, Name_id);
	fflush(stdout);
#endif
	/*
	 * add the cause_host frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't append cause_host frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s added cause_host to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the cause_seq_no
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", my_log_seq_no);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for cause_seq_no to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got frame for cause_seq_no %lu\n", woof_name, my_log_seq_no);
	fflush(stdout);
#endif
	/*
	 * add the cause_seq_no frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't append cause_seq_no frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s added cause_host to frame\n", woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s couldn't recv msg for seq_no %lu name %s to server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGet: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s no recv frame for seq_no %lu to server at %s\n",
					woof_name, seq_no, endpoint);
			perror("WooFMsgGet: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		r_size = zframe_size(r_frame);
		if (r_size == 0)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s no data in frame for seq_no %lu to server at %s\n",
					woof_name, seq_no, endpoint);
			fflush(stderr);
			zmsg_destroy(&r_msg);
			return (-1);
		}
		else
		{
			if (r_size > el_size)
			{
				r_size = el_size;
			}
			memcpy(element, zframe_data(r_frame), r_size);
#ifdef DEBUG
			printf("WooFMsgGet: woof: %s copied %lu bytes for seq_no %lu message to server at %s\n",
				   woof_name, r_size, seq_no, endpoint);
			fflush(stdout);

#endif
			zmsg_destroy(&r_msg);
		}
	}

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s recvd seq_no %lu message to server at %s\n",
		   woof_name, seq_no, endpoint);
	fflush(stdout);

#endif
	return (1);
}

#ifdef DONEFLAG
int WooFMsgGetDone(char *woof_name, unsigned long seq_no)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	int r_size;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;
	struct timeval tm;
	int err;
	unsigned long done;

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * assume it is local
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgGetDone: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a get done flag message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_GET_DONE);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s no frame for WOOF_MSG_GET_DONE command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetDone: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got WOOF_MSG_GET_DONE command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s can't append WOOF_MSG_GET_DONE command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetDone: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetDone: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGetDone: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetDone: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the seq_no
	 */

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", seq_no);
	frame = zframe_new(buffer, strlen(buffer));

	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s no frame for seq_no %lu server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGetDone: couldn't get new seq_no frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetDone: woof: %s got frame for seq_no %lu\n", woof_name, seq_no);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s couldn't append frame for seq_no %lu to server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGetDone: couldn't append seq_no frame");
		fflush(stderr);
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetDone: woof: %s appended frame for seq_no %lu\n", woof_name, seq_no);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGetDone: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgGetDone: woof: %s couldn't recv msg for seq_no %lu name %s to server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGetDone: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGetDone: woof: %s no recv frame for seq_no %lu to server at %s\n",
					woof_name, seq_no, endpoint);
			perror("WooFMsgGetDone: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		r_size = zframe_size(r_frame);
		if (r_size == 0)
		{
			fprintf(stderr, "WooFMsgGetDone: woof: %s no data in frame for seq_no %lu to server at %s\n",
					woof_name, seq_no, endpoint);
			fflush(stderr);
			zmsg_destroy(&r_msg);
			return (-1);
		}
		else
		{
			done = 0;
			memcpy(&done, zframe_data(r_frame), sizeof(done));
#ifdef DEBUG
			printf("WooFMsgGetDone: woof: %s got done: %lu for seq_no %lu message to server at %s\n",
				   woof_name, done, seq_no, endpoint);
			fflush(stdout);

#endif
			zmsg_destroy(&r_msg);
		}
	}

	if (done == 0)
	{
		return (0);
	}
	else if (done == 1)
	{
		return (1);
	}
	else
	{
		fprintf(stderr, "WooFMsgGetDone: bad done value from %s, seq_no %lu, done: %lu\n",
				woof_name, seq_no, done);
		fflush(stderr);
		return (-1);
	}
}

#endif

#ifdef REPAIR
unsigned long int LogGetRemoteSize(char *endpoint)
{
	int err;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	unsigned long copy_size;
	char buffer[255];
	unsigned long int size;

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "LogGetRemoteSize: no outbound msg to server at %s\n",
				endpoint);
		perror("LogGetRemoteSize: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("LogGetRemoteSize: got new msg\n");
	fflush(stdout);
#endif

	/*
	 * this is a LOG_GET_REMOTE_SIZE message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", LOG_GET_REMOTE_SIZE);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "LogGetRemoteSize: no frame for LOG_GET_REMOTE_SIZE command in to server at %s\n",
				endpoint);
		perror("LogGetRemoteSize: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("LogGetRemoteSize: got LOG_GET_REMOTE_SIZE command frame frame\n");
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "LogGetRemoteSize: can't append LOG_GET_REMOTE_SIZE command frame to msg for server at %s\n",
				endpoint);
		perror("LogGetRemoteSize: couldn't append command frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
     * get response
     */
	r_msg = ServerRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "LogGetRemoteSize: couldn't recv msg for log tail from server at %s\n",
				endpoint);
		perror("LogGetRemoteSize: no response received");
		fflush(stderr);
		return (-1);
	}
	r_frame = zmsg_first(r_msg);
	if (r_frame == NULL)
	{
		fprintf(stderr, "LogGetRemoteSize: no recv frame for log tail from server at %s\n",
				endpoint);
		perror("LogGetRemoteSize: no response frame");
		zmsg_destroy(&r_msg);
		return (-1);
	}
	size = strtoul(zframe_data(r_frame), (char **)NULL, 10);
	zmsg_destroy(&r_msg);

#ifdef DEBUG
	printf("LogGetRemoteSize: received log size from %s\n", endpoint);
	fflush(stdout);
#endif

	return size;
}

int LogGetRemote(LOG *log, MIO *mio, char *endpoint)
{
	int err;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	unsigned long copy_size;
	char buffer[255];

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "LogGetRemote: no outbound msg to server at %s\n",
				endpoint);
		perror("LogGetRemote: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("LogGetRemote: got new msg\n");
	fflush(stdout);
#endif

	/*
	 * this is a LOG_GET_REMOTE message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", LOG_GET_REMOTE);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "LogGetRemote: no frame for LOG_GET_REMOTE command in to server at %s\n",
				endpoint);
		perror("LogGetRemote: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("LogGetRemote: got LOG_GET_REMOTE command frame frame\n");
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "LogGetRemote: can't append LOG_GET_REMOTE command frame to msg for server at %s\n",
				endpoint);
		perror("LogGetRemote: couldn't append command frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
     * get response
     */
	r_msg = ServerRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "LogGetRemote: couldn't recv msg for log tail from server at %s\n",
				endpoint);
		perror("LogGetRemote: no response received");
		fflush(stderr);
		return (-1);
	}
	r_frame = zmsg_first(r_msg);
	if (r_frame == NULL)
	{
		fprintf(stderr, "LogGetRemote: no recv frame for log tail from server at %s\n",
				endpoint);
		perror("LogGetRemote: no response frame");
		zmsg_destroy(&r_msg);
		return (-1);
	}
	copy_size = zframe_size(r_frame);
	memcpy((void *)log, (void *)zframe_data(r_frame), copy_size);
	log->m_buf = mio;
	zmsg_destroy(&r_msg);

#ifdef DEBUG
	printf("LogGetRemote: received log from %s\n", endpoint);
	fflush(stdout);
#endif

	return (1);
}

/* TODO:
 * Start woof repair. The woof objects from begin_seq_no to end_seq_no are to be repaired.
 */
int WooFMsgRepair(char *woof_name, Dlist *holes)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	int r_size;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;
	unsigned long to_be_filled;
	unsigned long count;
	unsigned long *seq_no;
	DlistNode *dn;
	int i;
	struct timeval tm;
	int err;

	if (holes == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: hole list is NULL\n");
		fflush(stderr);
		return (-1);
	}
	count = holes->count;
	seq_no = malloc(count * sizeof(unsigned long));
	i = 0;
	DLIST_FORWARD(holes, dn)
	{
		seq_no[i++] = dn->value.i64;
	}

	memset(namespace, 0, sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s no name space\n", woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * assume it is local
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgRepair: woof: %s invalid IP address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(namespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a repair message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", WOOF_MSG_REPAIR);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s no frame for WOOF_MSG_REPAIR command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s got WOOF_MSG_REPAIR command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s can't append WOOF_MSG_REPAIR command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: couldn't append WOOF_MSG_REPAIR command frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the count
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", count);
	frame = zframe_new(buffer, strlen(buffer));

	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s no frame for count %lu server at %s\n",
				woof_name, count, endpoint);
		perror("WooFMsgRepair: couldn't get new count frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s got frame for count %lu\n", woof_name, count);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s couldn't append frame for count %lu to server at %s\n",
				woof_name, count, endpoint);
		perror("WooFMsgRepair: couldn't append count frame");
		fflush(stderr);
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s appended frame for count %lu\n", woof_name, count);
	fflush(stdout);
#endif

	/*
	 * make a frame for the seq_no
	 */
	frame = zframe_new(seq_no, count * sizeof(unsigned long));

	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s no frame for seq_no server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: couldn't get new seq_no frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s got frame for seq_no\n", woof_name);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s couldn't append frame for seq_no to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: couldn't append seq_no frame");
		fflush(stderr);
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s appended frame for seq_no\n", woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgRepair: woof: %s couldn't recv msg from server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgRepair: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgRepair: woof: %s no recv frame from server at %s\n",
					woof_name, endpoint);
			perror("WooFMsgRepair: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		// TODO:
		str = zframe_data(r_frame);
		to_be_filled = strtoul(str, (char **)NULL, 10);
		zmsg_destroy(&r_msg);
	}

#ifdef DEBUG
	printf("WooFMsgRepair: woof: %s recvd to_be_filled %lu message from server at %s\n",
		   woof_name, to_be_filled, endpoint);
	fflush(stdout);
#endif
	if (to_be_filled != count)
	{
		return (-1);
	}
	return (1);
}

#endif