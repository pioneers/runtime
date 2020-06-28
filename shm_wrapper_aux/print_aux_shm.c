#include "shm_wrapper_aux.h"

int main() {
    logger_init(TEST);
    shm_aux_init(TEST);
    print_robot_desc();
    print_gamepad_state();
}