#include <stdlib.h>
#include <unistd.h>
#include <zmq.h>

int main (void)
{
// Create frontend and backend sockets
        void *context = zmq_ctx_new();
	void *frontend = zmq_socket (context, ZMQ_PULL);
	void *backend = zmq_socket (context, ZMQ_PUSH);
// Bind both sockets to TCP ports
	zmq_bind (frontend, "tcp://*:6028");
	zmq_bind (backend, "tcp://*:6029");
// Start the queue proxy, which runs until ETERM 
	zmq_proxy (frontend, backend, NULL);

	return 0;
}


