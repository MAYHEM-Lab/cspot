#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>

int main (void)
{
	char *reply;
	zsock_t *receiver;

	zsock_t *push = zsock_new_req (">tcp://10.1.5.30:6029");
	zpoller_t *poller = zpoller_new(push,NULL);

	if(push == NULL) {
		fprintf(stderr,"new req failed\n");
		exit(1);
	}

	if(poller == NULL) {
		fprintf(stderr,"new req failed\n");
		exit(1);
	}

	zstr_send (push, "Hello, World\n");

	receiver = zpoller_wait(poller,10000);
	if(receiver != NULL) {
		reply = zstr_recv(push);
		printf("received: %s\n",reply);
		fflush(stdout);
		zstr_free(&reply);
	} else if(zpoller_expired(poller)) {
		printf("timeout expired\n");
		fflush(stdout);
	} else if(zpoller_terminated(poller)) {
		printf("terminated\n");
		fflush(stdout);
	}

	zsock_destroy (&push);
	return 0;
}


