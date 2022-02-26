#include "../test.h"

#define UID 0x123

int main() {
    start_test("Other Device Test", "", NO_REGEX);

    char* dev_name = "OtherTestDevice";
    connect_virtual_device(dev_name, UID);

    uint8_t dev_type = device_name_to_type(dev_name);
    device_t* dev = get_device(dev_type);

    // param_val_t device_values[dev->num_params];

    param_val_t constant_value = {.p_i = 100};

    same_param_value(dev_name, UID, "CONSTANT", INT, constant_value);

    return 0;
}