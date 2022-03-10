#include "../test.h"

#define UID 0x123

int main() {
    start_test("Other Device Test", "", NO_REGEX);

    char* dev_name = "OtherTestDevice";
    int socket = connect_virtual_device(dev_name, UID);
    sleep(1);

    check_device_connected(UID);
    printf("reached");
    sleep(1);
    disconnect_virtual_device(socket);
    sleep(1);
    check_device_not_connected(UID);

    connect_virtual_device(dev_name, UID);
    sleep(1);
    check_device_connected(UID);

    uint8_t dev_type = device_name_to_type(dev_name);
    device_t* dev = get_device(dev_type);

    // param_val_t device_values[dev->num_params];


    param_val_t constant_value = {.p_i = 100};

    same_param_value(dev_name, UID, "CONSTANT", INT, constant_value);

    return 0;
}