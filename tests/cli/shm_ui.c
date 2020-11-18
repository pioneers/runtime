/**
 * The UI for shared memory using ncurses.
 * This displays a live view of memory blocks in SHM.
 */
#include <curses.h>
#include <string.h>
#include <sys/wait.h>

#include "../../shm_wrapper/shm_wrapper.h"
#include "../../runtime_util/runtime_util.h"

// Constants for defining the refresh rate of the UI
// This is also how often we poll shared memory for updates
#define FPS 1

// ************************** SIZES AND POSITIONS *************************** //
#define ROBOT_DESC_HEIGHT 10
// ROBOT_DESC_WIDTH is enough to fit GAMEPAD_WIDTH
#define ROBOT_DESC_WIDTH 35
#define ROBOT_DESC_START_Y 1
#define ROBOT_DESC_START_X 1

// GAMEPAD_HEIGHT is enough to fit all the joysticks and buttons
#define GAMEPAD_HEIGHT 30
#define GAMEPAD_WIDTH ROBOT_DESC_WIDTH
#define GAMEPAD_START_Y (ROBOT_DESC_START_Y + ROBOT_DESC_HEIGHT)
#define GAMEPAD_START_X 1

#define DEVICE_HEIGHT (GAMEPAD_START_Y + GAMEPAD_HEIGHT - 1)
// DEVICE_WIDTH is enough to fit each line
#define DEVICE_WIDTH 90
#define DEVICE_START_Y 1
#define DEVICE_START_X (ROBOT_DESC_WIDTH + 1)

#define SUB_HEIGHT DEVICE_HEIGHT
#define SUB_WIDTH 50
#define SUB_START_Y 1
#define SUB_START_X (DEVICE_START_X + DEVICE_WIDTH)

// Column sizes in device window
const int value_width = strlen("123.000000") + 2; // String representation of float length
const int param_idx_col = 5;
const int param_name_col = param_idx_col + strlen("Param Idx") + 2;
const int command_val_col = param_name_col + strlen("increasing_even") + 2; // Sample long parameter name
const int data_val_col = command_val_col + value_width;

// ******************************** WINDOWS ********************************* //
WINDOW* robot_desc_win;
WINDOW* gamepad_win;
WINDOW* sub_win;
WINDOW* device_win;

// **************************** MISC GLOBAL VARS **************************** //
char** joystick_names;
char** button_names;
uint32_t catalog = 0; // Shared memory catalog (bitmap of connected devices)
dev_id_t dev_ids[MAX_DEVICES]; // Device identifying info on each shm-connected device

// ******************************** UTILS ********************************* //

// Returns the string representation of a bitmap; LSB is left most character
void bitmap_string(uint32_t bitmap, char str[]) {
    for (int i = 0; i < 32; i++) {
        if (bitmap & (1 < i)) {
            str[i] = '1';
        } else {
            str[i] = '0';
        }
    }
    str[32] = '\0';
}
// ***************************** SHARED MEMORY ****************************** //

void start_shm() {
    pid_t shm_pid;

    // fork shm_start process
    if ((shm_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (shm_pid == 0) {  // child
        // cd to the shm_wrapper directory
        if (chdir("../shm_wrapper") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // exec the shm_start process
        if (execlp("./shm_start", "shm", NULL) < 0) {
            printf("execlp: %s\n", strerror(errno));
        }
    } else {  // parent
        // wait for shm_start process to terminate
        if (waitpid(shm_pid, NULL, 0) < 0) {
            printf("waitpid shm: %s\n", strerror(errno));
        }

        // init to the now-existing shared memory
        shm_init();
    }
}

void stop_shm() {
    pid_t shm_pid;

    // fork shm_stop process
    if ((shm_pid = fork()) < 0) {
        printf("fork: %s\n", strerror(errno));
    } else if (shm_pid == 0) {  // child
        // cd to the shm_wrapper directory
        if (chdir("../shm_wrapper") == -1) {
            printf("chdir: %s\n", strerror(errno));
        }

        // exec the shm_stop process
        if (execlp("./shm_stop", "shm", NULL) < 0) {
            printf("execlp: %s\n", strerror(errno));
        }
    } else {  // parent
        // wait for shm_stop process to terminate
        if (waitpid(shm_pid, NULL, 0) < 0) {
            printf("waitpid shm: %s\n", strerror(errno));
        }
    }
}

// ******************************** NCURSES ********************************* //

// Initializes variables
void init() {
    // Names
    joystick_names = get_joystick_names();
    button_names = get_button_names();

    // Robot description
    robot_desc_win = newwin(ROBOT_DESC_HEIGHT, ROBOT_DESC_WIDTH, ROBOT_DESC_START_Y, ROBOT_DESC_START_X);
    mvwprintw(robot_desc_win, 1, 1, "~~~~~~~~Robot Description~~~~~~~~");
    // Gamepad
    gamepad_win = newwin(GAMEPAD_HEIGHT, GAMEPAD_WIDTH, GAMEPAD_START_Y, GAMEPAD_START_X);
    mvwprintw(gamepad_win, 1, 1, "~~~~~~~~~~Gamepad State~~~~~~~~~~");
    // Subscriptions (devices and parameters)
    sub_win = newwin(SUB_HEIGHT, SUB_WIDTH, SUB_START_Y, SUB_START_X);
    box(sub_win, 0, 0);
    // Device info (device data)
    device_win = newwin(DEVICE_HEIGHT, DEVICE_WIDTH, DEVICE_START_Y, DEVICE_START_X);
    mvwprintw(device_win, 1, 1, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Device Description~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

    refresh();
}

// Displays the robot description
void display_robot_desc() {
    // Gather info from SHM
    robot_desc_val_t run_mode = robot_desc_read(RUN_MODE);
    robot_desc_val_t dawn_connection = robot_desc_read(DAWN);
    robot_desc_val_t shepherd_connection = robot_desc_read(SHEPHERD);
    robot_desc_val_t gamepad_connection = robot_desc_read(GAMEPAD);
    robot_desc_val_t start_pos = robot_desc_read(START_POS);

    // Print each field (clear previous value before printing current value)
    int line = 3;
    wclrtoeol(robot_desc_win);
    mvwprintw(robot_desc_win, line++, 1, "RUN_MODE = %s", (run_mode == IDLE) ? "IDLE" : (run_mode == AUTO ? "AUTO" : (run_mode == TELEOP ? "TELEOP" : "CHALLENGE")));
    wclrtoeol(robot_desc_win);
    mvwprintw(robot_desc_win, line++, 1, "DAWN = %s", (dawn_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(robot_desc_win);
    mvwprintw(robot_desc_win, line++, 1, "SHEPHERD = %s", (shepherd_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(robot_desc_win);
    mvwprintw(robot_desc_win, line++, 1, "GAMEPAD = %s", (gamepad_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(robot_desc_win);
    mvwprintw(robot_desc_win, line++, 1, "START_POS = %s", (start_pos == LEFT) ? "LEFT" : "RIGHT");

    // Box and refresh
    box(robot_desc_win, 0, 0);
    wrefresh(robot_desc_win);
}

// Displays the gamepad state
void display_gamepad_state() {
    // Start at line after the header
    int line = 2;
    wmove(gamepad_win, line, 5);
    wclrtoeol(gamepad_win); // Clear "No gamepad connected"

    // Read gamepad state
    uint32_t pressed_buttons = 0;
    float joystick_vals[4];
    if (gamepad_read(&pressed_buttons, joystick_vals) != 0) {
        mvwprintw(gamepad_win, line, 5, "No gamepad connected!");
        for (int i = 0; i < 4; i++) {
            joystick_vals[i] = -1;
        }
    }
    line++;

    // Print joysticks
    mvwprintw(gamepad_win, line++, 1, "Joystick Positions:");
    for (int i = 0; i < 4; i++) {
        wclrtoeol(gamepad_win); // Clear previous joystick position
        if (joystick_vals[i] != -1) {
            mvwprintw(gamepad_win, line, 5, "%s = %f", joystick_names[i], joystick_vals[i]);
        }
        wmove(gamepad_win, ++line, 5);
    }
    line += 2;

    // Print pressed buttons
    mvwprintw(gamepad_win, line++, 1, "Pressed Buttons:");
    for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
        wclrtoeol(gamepad_win); // Clear button name
        if (pressed_buttons & (1 << i)) {
            // Show button name if pressed
            mvwprintw(gamepad_win, line, 5, "%s", button_names[i]);
        }
        wmove(gamepad_win, ++line, 5);
    }

    // Draw box and refresh
    box(gamepad_win, 0, 0);
    wrefresh(gamepad_win);
}

/**
 * Displays a device's current state of parameters
 * Arguments:
 *    shm_idx: the index of shared memory of the device to display
 */
void display_device(int shm_idx) {
    // Get newest shm data
    get_catalog(&catalog);
    get_device_identifiers(dev_ids);
    // Device is not connected at this shared memory index
    if (!(catalog & (1 << shm_idx))) {
        mvwprintw(device_win, DEVICE_HEIGHT - 2, 1, "Use the left and right arrow keys to inspect the previous or next device!");
        box(device_win, 0, 0);
        wrefresh(device_win);
        return;
    }
    int line = 3;
    const int table_header_line = line + 1;

    // Print current device
    device_t* device = get_device(dev_ids[shm_idx].type);
    wclrtoeol(device_win); // Clear previous string
    mvwprintw(device_win, line++, 1, "dev_ix = %d: name = %s, type = %d, year = %d, uid = %llu", shm_idx, device->name, dev_ids[shm_idx].type,  dev_ids[shm_idx].year,  dev_ids[shm_idx].uid);
    line += 2;

    // Print all command values and data values
    param_val_t command_vals[device->num_params];
    param_val_t data_vals[device->num_params];
    device_read(shm_idx, EXECUTOR, COMMAND, ~0, command_vals);
    device_read(shm_idx, EXECUTOR, DATA, ~0, data_vals);
    for (int i = 0; i < device->num_params; i++) {
        wclrtoeol(device_win); // Clear previous joystick position
        mvwprintw(device_win, line, param_idx_col, "%d", i);
        mvwprintw(device_win, line, param_name_col, "%s", device->params[i].name);
        switch (device->params[i].type) {
            case INT:
                mvwprintw(device_win, line, command_val_col, "%d", command_vals[i].p_i);
                mvwprintw(device_win, line, data_val_col, "%d", data_vals[i].p_i);
                break;
            case FLOAT:
                mvwprintw(device_win, line, command_val_col, "%f", command_vals[i].p_f);
                mvwprintw(device_win, line, data_val_col, "%f", data_vals[i].p_f);
                break;
            case BOOL:
                mvwprintw(device_win, line, command_val_col, "%s", command_vals[i].p_b ? "TRUE" : "FALSE");
                mvwprintw(device_win, line, data_val_col, "%s", data_vals[i].p_b ? "TRUE" : "FALSE");
                break;
        }
        wmove(gamepad_win, ++line, 5);
    }

    // Draw table with schema (param_idx (int), name (str), command (variable), data (variable))
    mvwprintw(device_win, table_header_line, param_idx_col, "Param Idx");
    mvwprintw(device_win, table_header_line, param_name_col, "Parameter Name");
    mvwprintw(device_win, table_header_line, command_val_col, "Command");
    mvwprintw(device_win, table_header_line, data_val_col, "Data");
    mvwhline(device_win, table_header_line + 1, param_idx_col, 0, data_val_col + value_width - 6); // Horizontal line
    // Vertical Lines
    mvwvline(device_win, table_header_line, param_name_col - 1, 0, MAX_PARAMS + 2);
    mvwvline(device_win, table_header_line, command_val_col - 1, 0, MAX_PARAMS + 2);
    mvwvline(device_win, table_header_line, data_val_col - 1, 0, MAX_PARAMS + 2);

    // Box and refresh
    mvwprintw(device_win, DEVICE_HEIGHT - 2, 1, "Use the left and right arrow keys to inspect the previous or next device!");
    box(device_win, 0, 0);
    wrefresh(device_win);
}

// Displays the changed and requested devices and params
void display_subscriptions() {
    int line = 1; // Line number is relative to the window
    mvwprintw(sub_win, line++, 1, "~~~~~~~~~~~~~~~~~SUBSCRIPTIONS~~~~~~~~~~~~~~~~~~");
    line++; // Leave a space between header and contents
    mvwprintw(sub_win, line++, 1, "Devices:");
    line++;
    mvwprintw(sub_win, line++, 1, "Parameters:");
    for (int i = 0; i < MAX_DEVICES; i++) {
        mvwprintw(sub_win, line++, 5, "Device %02d: %032d", i, 0); // TODO:
    }
    wrefresh(sub_win);
}

// ********************************** MAIN ********************************** //
void sigint_handler(int signum) {
    stop_shm();
    exit(0);
}

int main(int argc, char** argv) {
    // signal handler to clean up shm
    signal(SIGINT, sigint_handler);

    // Start shm
    start_shm();
    sleep(1); // ALlow shm to initialize

    // Start UI
    initscr();

    // Init variables
    init();

    // Update windows
    int loop_ctr = 0;
    while (1) {
        // Update UI
        display_robot_desc();
        display_gamepad_state();
        display_device(0); // TODO: Show device based on arrow keys
        display_subscriptions();
        // Throttle refresh rate
        sleep(1); // TODO: Use FPS
        mvprintw(0, 0, "loop %d; Catalog: 0x%016X", loop_ctr++, catalog);
        refresh();
    }

    // End UI on key press
    getch();
    stop_shm();
    endwin();
    return 0;
}
