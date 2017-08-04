#include <stdlib.h>
#include <unistd.h>
#include <zmq.h>
#include <czmq.h>

int main (void)
{

//	zsock_t *pull = zsock_new_pull (">tcp://10.1.5.30:6031");
	zsock_t *pull = zsock_new_rep ("@tcp://*:6029");

	if(pull == NULL) {
		fprintf(stderr,"zsock new failed\n");
		fflush(stderr);
		exit(1);
	}

	char *string = zstr_recv(pull);
	printf("%s",string);
	fflush(stdout);
	zstr_send(pull,"done");

	zstr_free (&string);
	zsock_destroy (&pull);

    return 0;
}


