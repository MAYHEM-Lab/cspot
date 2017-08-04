#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>

int main (void)
{
    char *reply;

    zsock_t *push = zsock_new_push (">tcp://10.1.5.30:6030");
//    zsock_t *push = zsock_new_push (">tcp://127.0.0.1:6028");
//    zsock_t *push = zsock_new_push (">tcp://172.17.0.3:6029");

//    zsock_bind (push, "tcp://127.0.0.1:%d", 6028);

    zstr_send (push, "Hello, World\n");
//    reply = zstr_recv(push);

    sleep(2);
//    zsock_unbind (push, "tcp://127.0.0.1:%d", 6028);
    zsock_destroy (&push);
    return 0;
}


