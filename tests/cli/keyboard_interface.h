#ifndef KEYBOARD_INTERFACE_H
#define KEYBOARD_INTERFACE_H

#include "../client/net_handler_client.h"

#include <arpa/inet.h>   // for inet_addr, bind, listen, accept, socket types
#include <netdb.h>       // for struct addrinfo
#include <netinet/in.h>  // for structures relating to IPv4 addresses
#include <stdio.h>

#define NUM_GAMEPAD_BUTTONS_AND_JOYSTICKS (NUM_GAMEPAD_BUTTONS + 2 * NUM_GAMEPAD_JOYSTICKS)

#define PORT 5006  // If running on docker, this port must be exposed in docker-compose.yml

// ********************************** MAIN PROCESS ****************************************** //

/**
 * Setups connection to keyboard_inputs python script. Processes incoming inputs and translates
 * it to gamepad and joystick info to send through the send_gamepad_state() function
 */
void setup_keyboard();

#endif