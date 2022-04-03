/* OtherTest Device - onboarding project? */
#include "virtual_device_util.h"

enum {
    VOLUME,
    GAIN,
    STATUS,
    CONSTANT
};


void init_params(param_val_t params[]) {
    params[VOLUME].p_f = 100.0;
    params[GAIN].p_f = 5.0;
    params[STATUS].p_b = 1;
    params[CONSTANT].p_i = 100;
}

void device_actions(param_val_t params[]) {
    // params[VOLUME].p_f += 5;
    params[GAIN].p_f += 1;
    params[STATUS].p_b = 1 - params[STATUS].p_b;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Incorrect number of arguments: %d out of %d\n", argc, 3);
        exit(1);
    }

    int fd = atoi(argv[1]);
    uint64_t uid = strtoull(argv[2], NULL, 0);

    uint8_t dev_type = device_name_to_type("OtherTestDevice");
    device_t* dev = get_device(dev_type);

    param_val_t params[dev->num_params];
    init_params(params);

    lowcar_protocol(fd, dev_type, dev_type, uid, params, &device_actions, 1000);
    return 0;
}