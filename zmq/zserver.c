#include <stdlib.h>
#include <unistd.h>
#include <zmq.h>
#include <czmq.h>

int main (void)
{

//	zsock_t *pull = zsock_new_pull (">tcp://10.1.5.30:6031");
	zsock_t *pull = zsock_new_rep ("@tcp://*:6029");
	zpoller_t *poller = zpoller_new(pull,NULL);
	zsock_t *reader;

	if(pull == NULL) {
		fprintf(stderr,"zsock new failed\n");
		fflush(stderr);
		exit(1);
	}

	if(poller == NULL) {
		fprintf(stderr,"poller new failed\n");
		fflush(stderr);
		exit(1);
	}

	reader = (zsock_t *)zpoller_wait(poller,10000);

	if(reader != NULL) {
		char *string = zstr_recv(reader);
		printf("%s",string);
		fflush(stdout);
		zstr_send(pull,"done");
		zstr_free (&string);
	} else if(zpoller_expired(poller)) {
		printf("poll expired\n");
		fflush(stdout);
	} else if(zpoller_terminated(poller)) {
		printf("interrupted\n");
		fflush(stdout);
	}

	zsock_destroy (&pull);

    return 0;
}


