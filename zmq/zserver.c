#include <stdlib.h>
#include <unistd.h>
#include <zmq.h>
#include <czmq.h>

int main (void)
{
//	zctx_t *context = zctx_new();
//	void *socket = zsocket_new(context,ZMQ_PULL);

	zsock_t *pull = zsock_new_pull (">tcp://10.1.5.30:6031");

	if(pull == NULL) {
		fprintf(stderr,"zsock new failed\n");
		fflush(stderr);
		exit(1);
	}

//	zsocket_bind(socket,"tcp://*:6029");
//	zsock_bind(pull,"tcp://10.1.5.30:6029");


	char *string = zstr_recv(pull);
//	char *string = zstr_recv(socket);
	printf("%s",string);
	fflush(stdout);

//    zstr_send(pull,"done");

	zstr_free (&string);
	zsock_destroy (&pull);
//	zsocket_destroy(context,socket);
//	zctx_destroy(&context);

    return 0;
}


