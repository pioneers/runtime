#include <czmq.h>
#include <zmq.h>

int main (void)
{
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    printf ("Current 0MQ version is %d.%d.%d\n", major, minor, patch);
    zsock_t *radio = zsock_new_radio ("inproc://zframe-test-radio");
    assert(radio);
    zsock_destroy(&radio);
    return 0;
}
