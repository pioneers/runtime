#include <czmq.h>
#include "types.h"

zframe_t *convert_to_zframe(int type, int size, char *data)
{
    char *str = (char*)malloc(size + 1);
    str[0] = (char)type;
    memcpy(str + 1, data, size);
    zframe_t *frame = zframe_new(str, size + 1);
    return frame;
}

int main()
{
    zsock_t *requester = zsock_new_push("tcp://192.168.0.24:5555");

    one_t a = {123, 4567};
    two_t b = {3.14159, 'x', 'b'};
    three_t c = {'z', 2.71828, 99999999};

    zframe_t *first = convert_to_zframe(1, sizeof(a), (char*)&a);
    assert(zframe_send(&first, requester, 0) == 0);
    sleep(1);
    zframe_t *second = convert_to_zframe(2, sizeof(b), (char*)&b);
    assert(zframe_send(&second, requester, 0) == 0);
    sleep(1);
    zframe_t *third = convert_to_zframe(3, sizeof(c), (char*)&c);
    assert(zframe_send(&third, requester, 0) == 0);

    zsock_destroy(&requester);
    return 0;
}
