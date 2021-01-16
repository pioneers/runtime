#ifndef KEYBOARD_INTERFACE_H
#define KEYBOARD_INTERFACE_H

#include "../client/net_handler_client.h"

#include <arpa/inet.h>   // for inet_addr, bind, listen, accept, socket types
#include <netdb.h>       // for struct addrinfo
#include <netinet/in.h>  // for structures relating to IPv4 addresses
#include <stdio.h>

#define NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS 32

#define PORT 5006  // If running on docker, this port must be exposed in docker-compose.yml
enum buttons {
    button_a,
    button_b,
    button_x,
    button_y,
    l_bumper,
    r_bumper,
    l_trigger,
    r_trigger,
    button_back,
    button_start,
    l_stick,
    r_stick,
    dpad_up,
    dpad_down,
    dpad_left,
    dpad_right,
    button_xbox,
    joystick_left_x_right,
    joystick_left_x_left,
    joystick_left_y_down,
    joystick_left_y_up,
    joystick_right_x_left,
    joystick_right_x_right,
    joystick_right_y_down,
    joystick_right_y_up
};

// ********************************** MAIN PROCESS ****************************************** //

/**
* Setups connection to keyboard_inputs python script. Processes incoming inputs and translates
* it to gamepad and joystick info to send through the send_gamepad_state() function
*/
void setup_keyboard();

#endif