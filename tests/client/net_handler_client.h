#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <stdbool.h>
#include <sys/wait.h>
#include "../../net_handler/net_util.h"

/**
 * Helper function to print out contents of a DevData message
 * Arguments:
 *    file: file to output printout to 
 *    dev_data: the unpacked device data message to print
 */
void print_dev_data(FILE* file, DevData* dev_data);

/**
 * Connects clients to an already existing instance of runtime.
 * Arguments:
 *    dawn: Whether to connect a fake Dawn
 *    shepherd: Whether to connect a fake Shepherd
 */
void connect_clients(bool dawn, bool shepherd);

/**
 * Starts a new instance of net handler and connects a fake Dawn and fake Shepherd.
 * Sets everything up for querying from the CLI or from a test.
 */
void start_net_handler();

/**
 * Sends SIGINT to the net handler process and then calls close_output()
 */
void stop_net_handler();

/**
 * Cleans up threads and closes open file descriptors 
 */
void close_output();

/**
 * Sends a Run Mode message from the specified client with the specified mode
 * Arguments:
 *    - client: one of SHEPHERD or DAWN
 *    - mode: one of IDLE, AUTO, or TELEOP
 * No return value.
 */
void send_run_mode(robot_desc_field_t client, robot_desc_val_t mode);

/**
* Sends a Game State message from Shepherd with the specified state
* Arguments:
*   - state: one of POISON_IVY, DEHYDRATION, or HYPOTHERMIA 
*/
void send_game_state(robot_desc_field_t state);

/**
 * Sends a Start Pos message from the specified client with the specified position
 * Arguments:
 *    - client: one of SHEPHERD or DAWN
 *    - pos: one of LEFT or RIGHT
 * No return value.
 */

void send_start_pos(robot_desc_field_t client, robot_desc_val_t pos);

/**
 * Sends a UserInput message from (fake) Dawn with the specified buttons pushed and joystick values
 * Arguments:
 *    - buttons: bitmap of which buttons are pressed. mappings are in runtime_util.h
 *    - joystick_vals[4]: values for the four joystick values. mappings are in runtime_util.h
 *    - source: which hardware source to send input as, GAMEPAD or KEYBOARD
 * No return value.
 */
void send_user_input(uint64_t buttons, float joystick_vals[4], robot_desc_field_t source);

/**
 * Sends a UserInput message from (fake) Dawn specifying that both Keyboard and Gamepad are disconnected.
 */
void disconnect_user_input();

/**
 * Calling this function will return the most recent device data packet coming into Dawn from Runtime.
 * NOTE: you must free the pointer returned by this function using dev_data__free_unpacked
 */
DevData* get_next_dev_data();

/**
 * Sends a Timestamp message with a "Dawn" timestamp attached to it. It is then received by the tcp_conn, where it 
 * sends a new Timestamp message with the "Runtime" timestamp attached to it. Finally it comes back around to "net_handler_client"
 * where it parses the message and calculates how much time it took to go to tcp_conn and back
 * No return value
 */
void send_timestamp();

#endif
