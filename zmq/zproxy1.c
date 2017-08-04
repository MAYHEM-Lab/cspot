#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>

int main (void)
{

	zsock_t *pull = zsock_new_pull ("@tcp://*:6030");
//	zsock_t *push = zsock_new_push (">tcp://172.17.0.3:6029");
	zsock_t *push = zsock_new_push (">tcp://127.0.0.1:6029");
	char *string;

	if(pull == NULL) {
		fprintf(stderr,"pull null\n");
		exit(1);
	}

	if(push == NULL) {
		fprintf(stderr,"push null\n");
		exit(1);
	}

	string = zstr_recv(pull);
	zstr_send(push,string);

	zsock_destroy(&pull);
	zsock_destroy(&push);
	

	return 0;
}


