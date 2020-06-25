#include <czmq.h>

int main (void)
{
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    printf ("Current 0MQ version is %d.%d.%d\n", major, minor, patch);
    printf("%d\n", zmq_has("draft"));
    zsock_t *radio = zsock_new_radio("udp://localhost:8000");
    assert(radio);
    zsock_destroy(&radio);
    return 0;
}
