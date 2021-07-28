#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <stdbool.h>
#include <sys/wait.h>
#include "../../net_handler/net_util.h"

typedef struct {
    uint64_t uid;     // what the uid of this device is
    char* name;       // name of this device ("KoalaBear", "LimitSwitch", etc.)
    uint32_t params;  // which params to subscribe to
} dev_subs_t;

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
 * Sends a Gamepad State message from Dawn over UDP with the specified buttons pushed and joystick values
 * Arguments:
 *    - buttons: bitmap of which buttons are pressed. mappings are in runtime_util.h
 *    - joystick_vals[4]: values for the four joystick values. mappings are in runtime_util.h
 *    - source: which hardware source to send input as, GAMEPAD or KEYBOARD
 * No return value.
 */
void send_user_input(uint64_t buttons, float joystick_vals[4], robot_desc_field_t source);

/**
 * Sends device subscriptions from Dawn over TCP with the specified device subscriptions
 * Arguments:
 *    - subs: contains the subscriptions for the devices' parameters
 *    - num_devices: contains number of devices for which we are sending subscription requests
 * No return value.
 */
void send_device_subs(dev_subs_t* subs, int num_devices);

/**
 * Sends a Timestamp message with a "Dawn" timestamp attached to it. It is then received by the tcp_conn, where it 
 * sends a new Timestamp message with the "Runtime" timestamp attached to it. Finally it comes back around to "net_handler_client"
 * where it parses the message and calculates how much time it took to go to tcp_conn and back
 * No return value
 */
void send_timestamp();

/**
 * Calling this function will return the most recent device data packet coming into Dawn from Runtime.
 * NOTE: you must free the pointer returned by this function using dev_data__free_unpacked
 */
DevData* get_next_dev_data();

/**
 * Updates the file pointer receiving TCP messages coming into Dawn or Shepherd.
 * Arguments:
 *    - output_address: address of the file stream for new output destination
 */
void update_tcp_output_fp(char* output_address);

#endif
