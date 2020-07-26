#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "../../runtime_util/runtime_util.h"
#include "../../net_handler/net_util.h"
#include <sys/time.h>
#include <sys/wait.h>

typedef struct dev_data {
	uint64_t uid;    //what the uid of this device is
	char *name;      //name of this devices ("KoalaBear", "LimitSwitch", etc.)
	uint32_t params; //which params to subscribe to
} dev_data_t;

/*
 * Starts the real net handler process and connects to all of its outputs
 * Sets everything up for querying from the CLI or from a test.
 */
void start_net_handler ();

/*
 * Sends SIGINT to the net handler process
 * Cleans up threads and closes open file descriptors
 */
void stop_net_handler ();

/*
 * Sends a Run Mode message from the specified client with the specified mode
 * Arguments:
 *    - int client: one of SHEPHERD_CLIENT or DAWN_CLIENT
 *    - int mode: one of IDLE_MODE, AUTO_MODE, or TELEOP_MODE
 * No return value.
 */
void send_run_mode (robot_desc_field_t client, robot_desc_val_t mode);

/*
 * Sends a Start Pos message from the specified client with the specified position
 * Arguments:
 *    - int client: one of SHEPHERD_CLIENT or DAWN_CLIENT
 *    - int pos: one of LEFT_POS or RIGHT_POS
 * No return value.
 */
void send_start_pos (robot_desc_field_t client, robot_desc_val_t pos);

/*
 * Sends a Gamepad State message from Dawn over UDP with the specified buttons pushed and joystick values
 * Arguments:
 *    - uintt32_t buttons: bitmap of which buttons are pressed. mappings are in runtime_util.h
 *    - float joystick_vals[4]: values for the four joystick values. mappings are in runtime_util.h
 * No return value.
 */
void send_gamepad_state (uint32_t buttons, float joystick_vals[4]);

/*
 * Sends a Challenge Data message from the specified client with the specified data
 * Arguments:
 *    - int client: one of SHEPHERD_CLIENT or DAWN_CLIENT
 *    - char **data: array of NUM_CHALLENGES strings, each containing the input to the corresponding challenge
 * No return value.
 */
void send_challenge_data (robot_desc_field_t client, char **data);

/*
 * Sends a Device Data message from Dawn over TCP with the specified device subscriptions
 * Arguments:
 *    - dev_data_t *data: contains the subscriptions for the devices' parameters
 *    - int num_devices: contains number of devices for which we are sending subscription requests
 * No return value.
 */
void send_device_data (dev_data_t *data, int num_devices);

/*
 * Calling this function will let the next device data packet coming into Dawn from Runtime
 * to be printed to standard output (since device data packets are constantly coming in, they
 * are normally suppressed; this function unsuppresses one single packet).
 */
void print_next_dev_data ();

#endif
