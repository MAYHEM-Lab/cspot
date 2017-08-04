#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>

int main (void)
{
    zsock_t *pull = zsock_new_pull (">tcp://127.0.0.1:6029");

    char *string = zstr_recv(pull);
    printf("%s",string);
    fflush(stdout);
//    zstr_send(pull,"done");

    zstr_free (&string);
    zsock_destroy (&pull);
    return 0;
}


