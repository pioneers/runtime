/**
 * Handles lowcar device connects/disconnects and
 * acts as the interface between the devices and shared memory
 */

#include <termios.h>  // for POSIX terminal control definitions in serialport_open()

#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"
#include "message.h"

/**
 * Each device will have a unique port number.
 * For example, if there is only one Arduino connected, it will (probably) appear as 
 * a file with path "/dev/ttyACM0". A second device connected will appear as
 * "/dev/ttyACM1".
 * Virtual devices (not Arduinos) on the other hand are UNIX sockets that appear as
 * "/tmp/ttyACM0"
 * In the code, the number is referred to as "port_num"
 * Depending on whether a device is an Arduino ("lowcar") or a virtual device,
 * dev handler has to open a connection with it differently.
 * 
 * These file paths may also be referred to as "port_prefix" in the code.
 */
#define LOWCAR_FILE_PATH "/dev/ttyACM"
#define VIRTUAL_FILE_PATH "/tmp/ttyACM"

// **************************** PRIVATE STRUCT ****************************** //

/* A struct shared between SENDER, RECEIVER, and RELAYER threads communicating
 * with the same device.
 * Contains information about each thread, how to communicate with the device,
 * and information about the device itself
 * The RELAYER thread is responsible for using this struct to properly clean up
 * when the device disconnects or times out
 */
typedef struct {
    pthread_t sender;                 // Thread to build and send outgoing messages
    pthread_t receiver;               // Thread to receive and process all incoming messages
    pthread_t relayer;                // Thread to get ACKNOWLEDGEMENT and monitor disconnect/timeout
    bool is_virtual;                  // True iff the device is a virtual device. Otherwise, an actual Arduino.
    uint8_t port_num;                 // The device is a file with path "<port_prefix><port_num>/"
    int file_descriptor;              // Obtained from opening port. Used to close port.
    int shm_dev_idx;                  // The unique index assigned to the device by shm_wrapper for shared memory operations on device_connect()
    dev_id_t dev_id;                  // set by relayer once ACKNOWLEDGEMENT is received
    uint64_t last_received_msg_time;  // set by receiver: Timestamp of the most recent message from the device
    pthread_mutex_t relay_lock;       // Mutex on relay->last_received_msg_time
    pthread_cond_t start_cond;        // Conditional variable for relayer to broadcast to sender and receiver to start work
} relay_t;

// ************************** FUNCTION DECLARATIONS ************************* //

// Main functions
void init();
void stop();
void poll_connected_devices();

// Polling Utility
int get_new_devices(uint32_t* lowcar_bitmap, uint32_t* virtual_bitmap);

// Threads for communicating with devices
void communicate(bool is_virtual, uint8_t port_num);
void* relayer(void* relay_cast);
void relay_clean_up(relay_t* relay);
void* sender(void* relay_cast);
void* receiver(void* relay_cast);

// Device communication
int send_message(relay_t* relay, message_t* msg);
int receive_message(relay_t* relay, message_t* msg);
int verify_device(relay_t* relay);

// Serial port or socket opening and closing
int connect_socket(const char* socket_name);
int serialport_open(const char* port_name);
int serialport_close(int fd);

// Utility
void cleanup_handler(void* args);

// **************************** GLOBAL VARIABLES **************************** //

// Bitmap indicating whether port "<port_prefix>*" is being monitored by dev handler, where * is the *-th bit
// Bits are turned on in get_new_devices() and turned off on disconnect/timeout in relay_clean_up()
uint32_t used_lowcar_ports = 0;
uint32_t used_virtual_ports = 0;
pthread_mutex_t used_ports_lock;  // poll_connected_devices() and relay_clean_up() shouldn't access used_ports at the same time

// ***************************** MAIN FUNCTIONS ***************************** //

// Initialize logger, shm, and mutexes
void init() {
    // Init logger
    logger_init(DEV_HANDLER);
    // Init shared memory
    shm_init();
    // Initialize lock on global variable USED_PORTS
    if (pthread_mutex_init(&used_ports_lock, NULL) != 0) {
        log_printf(FATAL, "init: Couldn't init USED_PORTS_LOCK");
        exit(1);
    }
}

// Disconnect devices from shared memory and destroy mutexes
void stop() {
    log_printf(INFO, "Interrupt received, terminating dev_handler\n");
    // For each tracked device, disconnect from shared memory
    uint32_t connected_devs = 0;
    get_catalog(&connected_devs);
    for (int i = 0; i < MAX_DEVICES; i++) {
        if ((1 << i) & connected_devs) {
            device_disconnect(i);
        }
    }
    // Destroy locks
    pthread_mutex_destroy(&used_ports_lock);
    exit(0);
}

/**
 * Detects when devices are connected
 * On Arduino device connect,
 * connect to shared memory and spawn three threads to communicate with the device
 */
void poll_connected_devices() {
    // Poll for newly connected devices and open threads for them
    log_printf(DEBUG, "Polling now for devices.\n");
    uint32_t new_lowcar_devs;
    uint32_t new_virtual_devs;
    while (1) {
        new_lowcar_devs = 0;
        new_virtual_devs = 0;
        if (get_new_devices(&new_lowcar_devs, &new_virtual_devs) > 0) {
            // If bit i of either bitmap is on, then it's a new device
            for (int i = 0; (new_lowcar_devs >> i) > 0 && i < MAX_DEVICES; i++) {
                if (new_lowcar_devs & (1 << i)) {
                    communicate(false, i);
                }
            }
            for (int i = 0; (new_virtual_devs >> i) > 0 && i < MAX_DEVICES; i++) {
                if (new_virtual_devs & (1 << i)) {
                    communicate(true, i);
                }
            }
        }
        // Save CPU usage by checking for new devices only every so often (defined in runtime_util.h)
        usleep(POLL_INTERVAL);
    }
}

// **************************** POLLING UTILITY **************************** //

/**
 * Helper function to scan for new devices.
 * Arguments:
 *    is_virtual: true iff this should scan for virtual devices (otherwise Arduinos)
 *    found_devices: Bit i will be on if a new device is connected at port i
 * Returns:
 *    the number of new devices found
 */
static int get_new_devices_helper(bool is_virtual, uint32_t* found_devices) {
    int num_devices_found = 0;
    uint32_t* used_ports = (is_virtual) ? &used_virtual_ports : &used_lowcar_ports;
    char device_path[15];
    for (int i = 0; i < MAX_DEVICES; i++) {
        pthread_mutex_lock(&used_ports_lock);
        // Check if i-th bit of USED_PORTS is zero (indicating device wasn't connected in previous function call)
        if (!(*used_ports & (1 << i))) {
            sprintf(device_path, "%s%d", is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, i);
            // If that port currently connected (file exists), it's a new device
            if (access(device_path, F_OK) != -1) {
                // Turn bit on
                *found_devices |= (1 << i);
                // Mark that we've taken care of this device
                *used_ports |= (1 << i);
                num_devices_found++;
            }
        }
        pthread_mutex_unlock(&used_ports_lock);
    }
    return num_devices_found;
}

/**
 * Finds which Arduinos and virtual devices are newly connected since the last call to this function
 * Arguments:
 *    lowcar_bitmap: Bit i will be turned on if <LOWCAR_FILE_PATH>[i] is a newly connected device
 *    virtual_bitmap: Bit i will be turned on if <VIRTUAL_FILE_PATH>[i] is a newly connected device
 * Returns:
 *    the number of devices that were found
 */
int get_new_devices(uint32_t* lowcar_bitmap, uint32_t* virtual_bitmap) {
    uint8_t num_devices_found = 0;
    num_devices_found += get_new_devices_helper(false, lowcar_bitmap);
    num_devices_found += get_new_devices_helper(true, virtual_bitmap);
    return num_devices_found;
}

// ******************************** THREADS ********************************* //

/**
 * Opens threads for communication with a device
 * Three threads will be opened:
 *  relayer: Verifies device is lowcar and cancels threads when device disconnects/timesout
 *  sender: Sends data to write to device and periodically sends DEVICE_PING
 *  receiver: Receives parameter data from the lowcar device and processes logs
 * Arguments:
 *    is_virtual: Whether the device is virtual
 *    port_num: The port number of the new device to connect to
 */
void communicate(bool is_virtual, uint8_t port_num) {
    relay_t* relay = malloc(sizeof(relay_t));
    if (relay == NULL) {
        log_printf(FATAL, "communicate: Failed to malloc");
        exit(1);
    }
    relay->is_virtual = is_virtual;
    relay->port_num = port_num;

    char port_name[15];  // Template size + 2 indices for port_number
    sprintf(port_name, "%s%d", is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
    if (relay->is_virtual) {  // Bind to socket
        relay->file_descriptor = connect_socket(port_name);
        if (relay->file_descriptor == -1) {
            log_printf(ERROR, "communicate: Couldn't connect to socket %s\n", port_name);
            relay_clean_up(relay);
            return;
        }
    } else {  // Open serial port
        relay->file_descriptor = serialport_open(port_name);
        if (relay->file_descriptor == -1) {
            log_printf(ERROR, "communicate: Couldn't open port %s\n", port_name);
            relay_clean_up(relay);
            return;
        }
    }

    // Initialize the other relay values
    relay->shm_dev_idx = -1;
    relay->dev_id.type = -1;
    relay->dev_id.year = -1;
    relay->dev_id.uid = -1;
    relay->last_received_msg_time = 0;
    pthread_mutex_init(&relay->relay_lock, NULL);
    pthread_cond_init(&relay->start_cond, NULL);

    // Open threads for sender, receiver, and relayer
    if (pthread_create(&relay->sender, NULL, sender, relay) != 0) {
        log_printf(ERROR, "communicate: Couldn't spawn thread for SENDER");
    }
    if (pthread_create(&relay->receiver, NULL, receiver, relay) != 0) {
        log_printf(ERROR, "communicate: Couldn't spawn thread for RECEIVER");
    }
    if (pthread_create(&relay->relayer, NULL, relayer, relay) != 0) {
        log_printf(ERROR, "communicate: Couldn't spawn thread for RELAYER");
    }
}

/**
 * Sends a DEVICE_PING to the device and waits for an ACKNOWLEDGEMENT
 * If the ACKNOWLEDGEMENT takes too long, close the device and exit all threads
 * Connects the device to shared memory and signals the sender and receiver to start
 * Continuously checks if the device disconnected or timed out
 *      If so, it disconnects the device from shared memory, closes the device, and frees memory
 * Arguments:
 *    relay_cast: uncasted relay_t struct containing device info
 */
void* relayer(void* relay_cast) {
    relay_t* relay = relay_cast;
    int ret;

    // Verify that the device is a lowcar device
    ret = verify_device(relay);
    if (ret != 0) {
        log_printf(DEBUG, "/dev/ttyACM%d couldn't be verified to be a lowcar device", relay->port_num);
        log_printf(ERROR, "A non-PiE device was recently plugged in. Please unplug immediately");
        relay_clean_up(relay);
        return NULL;
    }

    // At this point, the device is confirmed to be a lowcar device!

    // Connect the lowcar device to shared memory
    device_connect(&relay->dev_id, &relay->shm_dev_idx);
    if (relay->shm_dev_idx == -1) {
        relay_clean_up(relay);
        return NULL;
    }

    // Broadcast to the sender and receiver to start work
    pthread_cond_broadcast(&relay->start_cond);

    // If the device disconnects or times out, clean up
    log_printf(DEBUG, "Monitoring %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
    char port_name[14];
    sprintf(port_name, "%s%d", relay->is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
    while (1) {
        // If Arduino port file doesn't exist, it disconnected
        if (access(port_name, F_OK) == -1) {
            log_printf(INFO, "%s (0x%016llX) disconnected!", get_device_name(relay->dev_id.type), relay->dev_id.uid);
            relay_clean_up(relay);
            return NULL;
        }
        // If it took too long to receive a message, the device timed out
        pthread_mutex_lock(&relay->relay_lock);
        if ((millis() - relay->last_received_msg_time) >= TIMEOUT) {
            pthread_mutex_unlock(&relay->relay_lock);
            log_printf(WARN, "%s (0x%016llX) timed out!", get_device_name(relay->dev_id.type), relay->dev_id.uid);
            relay_clean_up(relay);
            return NULL;
        }
        pthread_mutex_unlock(&relay->relay_lock);
        usleep(POLL_INTERVAL);
    }
}

/**
 * Called by relayer to clean up after the device.
 * Closes serialport/socket, cancels threads,
 * disconnects from shared memory, and frees the RELAY struct
 * Arguments:
 *    relay: Struct containing device/thread info used to clean up
 */
void relay_clean_up(relay_t* relay) {
    // If couldn't connect to device in the first place, just mark as unused
    if (relay->file_descriptor == -1) {
        uint32_t* used_ports = relay->is_virtual ? &used_virtual_ports : &used_lowcar_ports;
        *used_ports &= ~(1 << relay->port_num);  // Set bit to 0 to indicate unused
        free(relay);
        // Sleep so that we don't spam attempts to connect to a possibly bad device
        sleep(TIMEOUT / 1000);
        return;
    }

    int ret;
    // Cancel the sender and receiver threads when ongoing transfers are completed
    pthread_cancel(relay->sender);
    pthread_cancel(relay->receiver);
    if ((ret = pthread_join(relay->sender, NULL)) != 0) {
        log_printf(ERROR, "relay_clean_up: pthread_join on sender failed -- error: %d", ret);
    }
    if ((ret = pthread_join(relay->receiver, NULL)) != 0) {
        log_printf(ERROR, "relay_clean_up: pthread_join on receiver failed -- error: %d", ret);
    }

    // Disconnect the device from shared memory if it's connected
    if (relay->shm_dev_idx != -1) {
        device_disconnect(relay->shm_dev_idx);
    }

    // Send a RST message to the device to signal that we are closing the connection
    message_t* rst = make_rst();
    ret = send_message(relay, rst);
    if (ret != 0) {
        log_printf(WARN, "Couldn't send RST to %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
    }
    destroy_message(rst);

    // Close the device
    serialport_close(relay->file_descriptor);

    // Mark that the device is disconnected in the global bitmap
    if ((ret = pthread_mutex_lock(&used_ports_lock))) {
        log_printf(ERROR, "relay_clean_up: used_ports_lock mutex lock failed with code %d", ret);
    }
    uint32_t* used_ports = relay->is_virtual ? &used_virtual_ports : &used_lowcar_ports;
    *used_ports &= ~(1 << relay->port_num);  // Set bit to 0 to indicate unused

    /// Clean up relay struct
    pthread_mutex_unlock(&used_ports_lock);
    pthread_mutex_destroy(&relay->relay_lock);
    pthread_cond_destroy(&relay->start_cond);
    if (relay->dev_id.uid == -1) {
        log_printf(DEBUG, "Cleaned up bad device %s%d\n", relay->is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
    } else {
        log_printf(DEBUG, "Cleaned up %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
    }
    free(relay);
}

/**
 * Continuously sends DEVICE_PING and reads from shared memory to send DEVICE_WRITE
 * Arguments:
 *    relay_cast: Uncasted relay_t struct containing device info
 */
void* sender(void* relay_cast) {
    relay_t* relay = relay_cast;

    // Wait until relayer gets an ACKNOWLEDGEMENT
    pthread_mutex_lock(&relay->relay_lock);
    pthread_cleanup_push(&cleanup_handler, (void*) relay);
    pthread_cond_wait(&relay->start_cond, &relay->relay_lock);
    pthread_cleanup_pop(1);

    // Cancel this thread only where pthread_testcancel()
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    // Start doing work
    uint32_t pmap[MAX_DEVICES + 1];
    param_val_t* params = malloc(MAX_PARAMS * sizeof(param_val_t));  // Array of params to be filled on device_read()
    if (params == NULL) {
        log_printf(FATAL, "sender: Failed to malloc");
        exit(1);
    }
    message_t* msg;  // Message to build
    int ret;         // Hold the value from send_message()
    uint64_t last_sent_ping_time = millis();
    while (1) {
        // Write to device if needed via a DEVICE_WRITE message
        get_cmd_map(pmap);
        if (pmap[0] & (1 << relay->shm_dev_idx)) {  // If bit i in pmap[0] != 0, there are values to write to device i
            // Read the new parameter values to write from shared memory as DEV_HANDLER from the COMMAND stream
            device_read(relay->shm_dev_idx, DEV_HANDLER, COMMAND, pmap[1 + relay->shm_dev_idx], params);
            // Serialize and bulk transfer a DeviceWrite packet with PARAMS to the device
            msg = make_device_write(relay->dev_id.type, pmap[1 + relay->shm_dev_idx], params);
            ret = send_message(relay, msg);
            if (ret != 0) {
                log_printf(WARN, "Couldn't send DEVICE_WRITE to %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
            }
            destroy_message(msg);
        }

        // Send another DEVICE_PING every PING_FREQ milliseconds
        if ((millis() - last_sent_ping_time) >= PING_FREQ) {
            msg = make_ping();
            ret = send_message(relay, msg);
            if (ret != 0) {
                log_printf(WARN, "Couldn't send DEVICE_PING to %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
            }
            // Update the timestamp at which we sent a DEVICE_PING
            last_sent_ping_time = millis();
            destroy_message(msg);
        }

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();  // Cancellation point
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        // Sleep to throttle this loop; Determines how frequently we can send messages to the device
        usleep(1000);
    }
    return NULL;
}

/**
 * Continuously attempts to parse incoming data over serial and send to shared memory
 * Sets relay->last_received_msg_time upon receiving a message
 * Arguments:
 *    relay_cast: uncasted relay_t struct containing device info
 */
void* receiver(void* relay_cast) {
    relay_t* relay = relay_cast;

    // Wait until relayer gets an ACKNOWLEDGEMENT
    pthread_mutex_lock(&relay->relay_lock);
    pthread_cleanup_push(&cleanup_handler, (void*) relay);
    pthread_cond_wait(&relay->start_cond, &relay->relay_lock);
    pthread_cleanup_pop(1);

    // Cancel this thread only where pthread_testcancel()
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    // Start doing work!
    // An empty message to parse the received data into
    message_t* msg = make_empty(MAX_PAYLOAD_SIZE);
    // An array of empty parameter values to be populated from DeviceData message payloads and written to shared memory
    param_val_t* vals = malloc(MAX_PARAMS * sizeof(param_val_t));
    if (vals == NULL) {
        log_printf(FATAL, "receiver: Failed to malloc");
        exit(1);
    }
    while (1) {
        // Try to read a message
        // Since this function blocks the thread until a message is received, we don't need to sleep in this loop
        if (receive_message(relay, msg) != 0) {
            // Message was broken... try to read the next message
            continue;
        }
        if (msg->message_id == DEVICE_DATA || msg->message_id == LOG || msg->message_id == DEVICE_PING) {
            // Update last received message time
            pthread_mutex_lock(&relay->relay_lock);
            relay->last_received_msg_time = millis();
            pthread_mutex_unlock(&relay->relay_lock);
            // Handle message
            if (msg->message_id == DEVICE_DATA) {
                // If received DEVICE_DATA, write to shared memory
                parse_device_data(relay->dev_id.type, msg, vals);  // Get param values from payload
                device_write(relay->shm_dev_idx, DEV_HANDLER, DATA, *((uint32_t*) msg->payload), vals);
            } else if (msg->message_id == LOG) {
                // If received LOG, send it to the logger
                log_printf(DEBUG, "[%s (0x%016llX)]: %s", get_device_name(relay->dev_id.type), relay->dev_id.uid, msg->payload);
            }
            // Device is going to disconnect, so we clean up on our end
        } else if (msg->message_id == RST) {
            relay_clean_up(relay);
            return NULL;
        } else {  // Invalid message type
            log_printf(WARN, "Dropped bad message (type %d) from %s (0x%016llX)", msg->message_id, get_device_name(relay->dev_id.type), relay->dev_id.uid);
        }
        // Now that the message is taken care of, clear the message
        msg->message_id = 0x0;
        msg->payload_length = 0;
        msg->max_payload_length = MAX_PAYLOAD_SIZE;
        memset(msg->payload, 0, MAX_PAYLOAD_SIZE);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();  // Cancellation point
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }
    return NULL;
}

// ************************** DEVICE COMMUNICATION ************************** //

/**
 * Helper function for sender()
 * Serializes, encodes, and sends a message
 * Arguments:
 *    relay: Contains the file descriptor
 *    msg: The message to be sent
 * Returns:
 *    0 if successful
 *    -1 if couldn't write all the bytes
 */
int send_message(relay_t* relay, message_t* msg) {
    int len = calc_max_cobs_msg_length(msg);
    uint8_t* data = malloc(len);
    if (data == NULL) {
        log_printf(FATAL, "send_message: Failed to malloc");
        exit(1);
    }
    len = message_to_bytes(msg, data, len);
    int transferred = writen(relay->file_descriptor, data, len);
    if (transferred != len) {
        log_printf(WARN, "Sent only %d out of %d bytes to %d (0x%016llX)\n", transferred, len, get_device_name(relay->dev_id.type), relay->dev_id.uid);
    }
    free(data);
    return (transferred == len) ? 0 : -1;
}

/**
 * Helper function for receiver()
 * Continuously reads from stream until reads the next message, then attempts to parse
 * This function blocks until it reads a (possibly broken) message
 * Arguments:
 *    relay: Contains the file descriptor and port number of the device
 *    msg: The message_t *to be populated with the parsed data (if successful)
 * Returns:
 *    0 on successful parse
 *    1 on broken message
 *    2 on incorrect checksum
 *    3 on timeout
 */
int receive_message(relay_t* relay, message_t* msg) {
    uint8_t last_byte_read = 0;  // Variable to temporarily hold a read byte
    int num_bytes_read = 0;

    if (relay->dev_id.uid == -1) {
        /* Haven't verified device is lowcar yet
         * read() is set to timeout while waiting for an ACK (see serialport_open())*/
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        num_bytes_read = read(relay->file_descriptor, &last_byte_read, 1);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (num_bytes_read == -1) {
            log_printf(ERROR, "receive_message: Error on read() for ACK--%s", strerror(errno));
            return 3;
        } else if (num_bytes_read == 0) {  // read() returned due to timeout
            log_printf(WARN, "Timed out when waiting for ACK from %s%d!", relay->is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
            return 3;
        } else if (last_byte_read != 0x00) {
            // If the first thing received isn't a perfect ACK, we won't accept it
            log_printf(WARN, "Attempting to read delimiter but got 0x%02X from %s%d\n", last_byte_read, relay->is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
            return 1;
        }
    } else {  // Receiving from a verified device
        // Keep reading a byte until we get the delimiter byte
        while (1) {
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            num_bytes_read = readn(relay->file_descriptor, &last_byte_read, 1);  // Waiting for first byte can block
            if (num_bytes_read == 0) {
                // received EOF so sleep to make device disconnected
                sleep(TIMEOUT / 1000 + 1);
                return 1;
            } else if (num_bytes_read == -1) {
                log_printf(ERROR, "receive_message: error reading from file: %s", strerror(errno));
                return 1;
            }
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            if (last_byte_read == 0x00) {
                // Found start of a message
                break;
            }
            // If we were able to read a byte but it wasn't the delimiter
            log_printf(WARN, "Attempting to read delimiter but got 0x%02X from %s%d\n", last_byte_read, relay->is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
        }
    }

    // Read the next byte, which tells how many bytes left are in the message
    uint8_t cobs_len;
    num_bytes_read = readn(relay->file_descriptor, &cobs_len, 1);
    if (num_bytes_read != 1) {
        return 1;
    } else if (cobs_len > (MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + MAX_PAYLOAD_SIZE + CHECKSUM_SIZE + 1)) {  // + 1 for cobs encoding overhead
        // Got some weird message that is unusually long (longer than a valid message with the longest payload)
        log_printf(WARN, "Received a cobs length that is too large");
        return 1;
    } else if (cobs_len < (MESSAGE_ID_SIZE + PAYLOAD_LENGTH_SIZE + CHECKSUM_SIZE + 1)) {  // + 1 for cobs encoding overhead
        // Got some weird message that is unusually short (shorter than a DEVICE_PING with no payload)
        log_printf(WARN, "Received a cobs length that is too small");
        return 1;
    }

    // Allocate buffer to read message into
    uint8_t* data = malloc(DELIMITER_SIZE + COBS_LENGTH_SIZE + cobs_len);
    if (data == NULL) {
        log_printf(FATAL, "receive_message: Failed to malloc");
        exit(1);
    }
    data[0] = 0x00;
    data[1] = cobs_len;

    // Read the message
    num_bytes_read = readn(relay->file_descriptor, &data[2], cobs_len);
    if (num_bytes_read != cobs_len) {
        log_printf(WARN, "Read only %d out of %d bytes from %s (0x%016llX)\n", num_bytes_read, cobs_len, get_device_name(relay->dev_id.type), relay->dev_id.uid);
        free(data);
        return 1;
    }

    // Parse the message
    int ret = parse_message(data, msg);
    free(data);
    if (ret != 0) {
        log_printf(WARN, "Couldn't parse message from %s%d\n", relay->is_virtual ? VIRTUAL_FILE_PATH : LOWCAR_FILE_PATH, relay->port_num);
        return 2;
    }
    return 0;
}

/**
 * Sends a DEVICE_PING to the device and waits for an ACKNOWLEDGEMENT
 * The first message received must be a perfectly constructed ACKNOWLEDGEMENT
 * Arguments:
 *    relay: Struct containing all relevant port information.
 *           dev_id field will be populated on successful ACKNOWLEDGEMENT
 * Returns:
 *    0 if received ACKNOWLEDGEMENT. Sets relay->dev_id
 *    1 if Ping message couldn't be sent
 *    2 if ACKNOWLEDGEMENT wasn't received
 */
int verify_device(relay_t* relay) {
    // Send a DEVICE_PING
    message_t* ping = make_ping();
    int ret = send_message(relay, ping);
    destroy_message(ping);
    if (ret != 0) {
        return 1;
    }

    // Try to read an ACKNOWLEDGEMENT, which we expect from a lowcar device that receives a DEVICE_PING
    message_t* ack = make_empty(MAX_PAYLOAD_SIZE);
    ret = receive_message(relay, ack);
    if (ret != 0) {
        log_printf(DEBUG, "Didn't receive ACK");
        destroy_message(ack);
        return 2;
    } else if (ack->message_id != ACKNOWLEDGEMENT) {
        log_printf(DEBUG, "Message is not an ACK, but of type %d", ack->message_id);
        destroy_message(ack);
        return 2;
    }

    // We have a lowcar device!

    /* Set serial port options to allow read() to block indefinitely
     * We expect the lowcar device to continuously send data
     * In serialport_open(), we set read() to timeout specifically for waiting for ACK */
    if (!relay->is_virtual) {
        struct termios toptions;
        if (tcgetattr(relay->file_descriptor, &toptions) < 0) {  // Get current options
            log_printf(ERROR, "verify_lowcar: Couldn't get term attributes for %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
            return -1;
        }
        toptions.c_cc[VMIN] = 1;  // read() must read at least a byte before returning
        // Save changes to TOPTIONS immediately using flag TCSANOW
        tcsetattr(relay->file_descriptor, TCSANOW, &toptions);
        if (tcsetattr(relay->file_descriptor, TCSAFLUSH, &toptions) < 0) {
            log_printf(ERROR, "verify_lowcar: Couldn't set term attributes for %s (0x%016llX)", get_device_name(relay->dev_id.type), relay->dev_id.uid);
            return -1;
        }
    }

    // Parse ACKNOWLEDGEMENT payload into relay->dev_id_t
    memcpy(&relay->dev_id.type, &ack->payload[0], 1);
    memcpy(&relay->dev_id.year, &ack->payload[1], 1);
    memcpy(&relay->dev_id.uid, &ack->payload[2], 8);
    log_printf(INFO, "Connected %s (0x%016llX) from year %d!", get_device_name(relay->dev_id.type), relay->dev_id.uid, relay->dev_id.year);
    relay->last_received_msg_time = millis();
    destroy_message(ack);
    return 0;
}

// ************************* SOCKETS / SERIAL PORTS ************************* //

/**
 * Binds to a socket for reading and writing binary data
 * Arguments:
 *     socket_name: THe name of the socket (ex: "/tmp/ttyACM0")
 * Returns:
 *    A valid file_descriptor, or
 *    -1 on error
 */
int connect_socket(const char* socket_name) {
    // Make a local socket for sending/receiving raw byte streams
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        log_printf(ERROR, "connect_socket: Couldn't create socket--%s", strerror(errno));
        return -1;
    }
    // Connect the socket to the found device's socket address
    // https://www.man7.org/linux/man-pages/man7/unix.7.html
    struct sockaddr_un dev_socket_addr = {0};
    dev_socket_addr.sun_family = AF_UNIX;
    strcpy(dev_socket_addr.sun_path, socket_name);
    if (connect(fd, (struct sockaddr*) &dev_socket_addr, sizeof(dev_socket_addr)) != 0) {
        log_printf(ERROR, "connect_socket: Couldn't connect socket %s--%s", dev_socket_addr.sun_path, strerror(errno));
        remove(socket_name);
        return -1;
    }

    // Set read() to timeout for up to TIMEOUT milliseconds
    struct timeval tv;
    tv.tv_sec = TIMEOUT / 1000;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv));
    return fd;
}

/**
 * Opens a serial port for reading and writing binary data
 * Uses 8-N-1 serial port config and without special processing
 * Also makes read() block for TIMEOUT milliseconds
 *      Used to timeout when waiting for an ACKNOWLEDGEMENT
 *      After receiving an ACKNOWLEDGEMENT, read() blocks until receiving at least a byte (set in verify_lowcar())
 * Arguments:
 *    port_name: The name of the port (ex: "/dev/ttyACM0", "/dev/tty.usbserial", "COM1")
 * Returns:
 *    A valid file_descriptor, or
 *    -1 on error
 */
int serialport_open(const char* port_name) {
    // Open the serialport for reading and writing
    // Need to specify O_NOCTTY to prevent attaching devices from becoming controlling terminals; see wiki
    int fd = open(port_name, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        log_printf(ERROR, "serialport_open: Unable to open port %s", port_name);
        return -1;
    }

    // Get the current serialport options
    struct termios toptions;
    if (tcgetattr(fd, &toptions) < 0) {
        log_printf(ERROR, "serialport_open: Couldn't get term attributes for port %s", port_name);
        return -1;
    }

    // Set the baudrate of communication to 115200 (same as on Arduino)
    speed_t brate = B115200;
    cfsetspeed(&toptions, brate);

    // Update serialport options: https://linux.die.net/man/3/cfsetspeed
    // Set serialport config to 8-N-1, which is default for Arduino Serial.begin()
    toptions.c_cflag &= ~CSIZE;   // Reset character size
    toptions.c_cflag |= CS8;      // Set character size to 8
    toptions.c_cflag &= ~PARENB;  // Disable parity generation on output and parity checking for input (N)
    toptions.c_cflag &= ~CSTOPB;  // Set only one stop bit (1)

    // Disables special processing of input and output bytes. See https://linux.die.net/man/3/cfsetspeed
    cfmakeraw(&toptions);

    // Set options for read(fd, buffer, num_bytes_to_read)
    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    toptions.c_cc[VMIN] = 0;               // Until receiving ACK, do not block indefinitely (use timeout)
    toptions.c_cc[VTIME] = TIMEOUT / 100;  // Number of deciseconds to timeout

    // Save changes to TOPTIONS. (Flag TCSANOW saves immediately)
    tcsetattr(fd, TCSANOW, &toptions);
    if (tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
        log_printf(ERROR, "serialport_open: Couldn't set term attributes for port %s", port_name);
        return -1;
    }

    return fd;
}

/**
 * Closes the serial port opened via serialport_open()
 * Arguments:
 *    fd: File descriptor obtained from serialport_open()
 * Returns:
 *    0 on success
 *    -1 on error and sets errno
 */
int serialport_close(int fd) {
    return close(fd);
}

// ******************************** UTILITY ********************************* //

/**
 * If sender/receive is canceled during pthread_cond_wait(), pthread_cleanup_push()
 * using this function guarantees that relay->relay_lock is unlocked before cancellation.
 * Arguments:
 *    args: Uncasted relay_t containing mutex requiring unlocking
 */
void cleanup_handler(void* args) {
    relay_t* relay = (relay_t*) args;
    pthread_mutex_unlock(&relay->relay_lock);
}

// ********************************** MAIN ********************************** //

int main(int argc, char* argv[]) {
    // If SIGINT (Ctrl+C) is received, call stop() to clean up
    signal(SIGINT, stop);
    init();
    log_printf(INFO, "DEV_HANDLER initialized.");
    poll_connected_devices();
    return 0;
}
