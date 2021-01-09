/**
 * The UI for shared memory using ncurses, a third-party dependency (not required to run runtime)
 * This displays a live view of memory blocks in SHM by polling frequently.
 * 
 * Important blocks of shared memory each have a dedicated ncurses "window"
 * 
 * Note that messages sent to stdout will interfere with the ui.
 * 
 * This implementation revolves around displaying strings at specified rows and columns,
 * which means we need to do some math with window dimensions. (See #define's below)
 * 
 * The general "template" for updating a window:
 *    - Get the current shared memory data
 *    - Display any "headers" (ex: the name of the window)
 *    - Initialize a pointer to where we need to start displaying
 *    - for each line of data:
 *         clear the line (Old data from the previous refresh)
 *         display information
 *         increment our line pointer
 *    - Redraw the box around the window (which is fragmented due to line clearing in the for-loop)
 *    - refresh the screen with our new changes
 * 
 * Install ncurses with:
 *    sudo apt-get install libncurses5-dev libncursesw5-dev
 */
#include <curses.h>
#include <string.h>
#include <sys/wait.h>

#include "../../shm_wrapper/shm_wrapper.h"
#include "../../runtime_util/runtime_util.h"
#include "../../logger/logger.h"

// The refresh rate of the UI and how often we poll shared memory data
#define FPS 2

// ******************************** WINDOWS ********************************* //
WINDOW* ROBOT_DESC_WIN; // Displays the robot description (clients connected, run mode, start position, gamepad connected)
WINDOW* GAMEPAD_WIN;    // Displays gamepad state (joystick values and what buttons are pressed)
WINDOW* SUB_WIN;        // Displays subscriptions from Dawn/Executor for each device
WINDOW* DEVICE_WIN;     // Displays device information (id and data) for a single device at a time based on user input

// ************************** SIZES AND POSITIONS *************************** //
// Some windows' dimensions are defined in terms of others so that their borders align

// Enough to fit each value
#define ROBOT_DESC_HEIGHT 10
// ROBOT_DESC_WIDTH is enough to fit GAMEPAD_WIDTH (which needs to be wider than ROBOT_DESC)
#define ROBOT_DESC_WIDTH 35
#define ROBOT_DESC_START_Y 1
#define ROBOT_DESC_START_X 1

// GAMEPAD_HEIGHT is enough to fit all the joysticks and buttons
#define GAMEPAD_HEIGHT 30
#define GAMEPAD_WIDTH ROBOT_DESC_WIDTH
// Display GAMEPAD_WIN below ROBOT_DESC
#define GAMEPAD_START_Y (ROBOT_DESC_START_Y + ROBOT_DESC_HEIGHT)
#define GAMEPAD_START_X 1

// Make the bottom border of DEVICE_WIN align with GAMEPAD_WIN's bottom border
#define DEVICE_HEIGHT (GAMEPAD_START_Y + GAMEPAD_HEIGHT - 1)
// DEVICE_WIDTH is enough to fit the information for each parameter
#define DEVICE_WIDTH 90
#define DEVICE_START_Y 1
// Make the left border of DEVICE_WIN to the right of ROBOT_DESC_WIN and GAMEPAD_WIN
#define DEVICE_START_X (ROBOT_DESC_WIDTH + 1)

// Make the top and bottom border of SUB_WIN align with that of DEVICE_WIN's
#define SUB_HEIGHT DEVICE_HEIGHT
#define SUB_WIDTH 50
#define SUB_START_Y 1
// Make the left border of SUB_WINn to the right of DEVICE_WIN
#define SUB_START_X (DEVICE_START_X + DEVICE_WIDTH)

// Column positions in device window; Declared as const instead of #define to prevent repeated strlen() computation
// Each one is determined by taking the previous column and adding the previous column's maximum width
const int VALUE_WIDTH = strlen("123.000000") + 2; // String representation of float length; Used to determine column widths
const int PARAM_IDX_COL = 5; // The column at which we display the parameter index
const int PARAM_NAME_COL = PARAM_IDX_COL + strlen("Param Idx") + 2; // The column at which we display the parameter name
const int COMMAND_VAL_COL = PARAM_NAME_COL + strlen("increasing_even") + 2; // The column at which we display the command stream
const int DATA_VAL_COL = COMMAND_VAL_COL + VALUE_WIDTH; // The column at which we display the data stream

// **************************** MISC GLOBAL VARS **************************** //

int DEVICE_WIN_IS_BLANK = 0; // Bool of whether the DEVICE_WIN is currently blank; See logic in display_device()

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

// Initializes ncurses windows
void init_windows() {
    // Robot description
    ROBOT_DESC_WIN = newwin(ROBOT_DESC_HEIGHT, ROBOT_DESC_WIDTH, ROBOT_DESC_START_Y, ROBOT_DESC_START_X);
    mvwprintw(ROBOT_DESC_WIN, 1, 1, "~~~~~~~~Robot Description~~~~~~~~");
    // Gamepad
    GAMEPAD_WIN = newwin(GAMEPAD_HEIGHT, GAMEPAD_WIDTH, GAMEPAD_START_Y, GAMEPAD_START_X);
    mvwprintw(GAMEPAD_WIN, 1, 1, "~~~~~~~~~~Gamepad State~~~~~~~~~~");
    // Subscriptions (devices and parameters)
    SUB_WIN = newwin(SUB_HEIGHT, SUB_WIDTH, SUB_START_Y, SUB_START_X);
    mvwprintw(SUB_WIN, 1, 1, "~~~~~~~~~~~~~~~~~Subscriptions~~~~~~~~~~~~~~~~~~");
    // Device info (device data)
    DEVICE_WIN = newwin(DEVICE_HEIGHT, DEVICE_WIDTH, DEVICE_START_Y, DEVICE_START_X);
    mvwprintw(DEVICE_WIN, 1, 1, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Device Description~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

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
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, 1, "RUN_MODE = %s", (run_mode == IDLE) ? "IDLE" : (run_mode == AUTO ? "AUTO" : (run_mode == TELEOP ? "TELEOP" : "CHALLENGE")));
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, 1, "DAWN = %s", (dawn_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, 1, "SHEPHERD = %s", (shepherd_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, 1, "GAMEPAD = %s", (gamepad_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, 1, "START_POS = %s", (start_pos == LEFT) ? "LEFT" : "RIGHT");

    // Box and refresh
    box(ROBOT_DESC_WIN, 0, 0);
    wrefresh(ROBOT_DESC_WIN);
}

/**
 * Reads the current gamepad state and displays it.
 * Arguments:
 *    joystick_names: the array of the joystick names
 *    button_names: the array of button names
 */
void display_gamepad_state(char** joystick_names, char** button_names) {
    // Start at line after the header
    int line = 2;
    wmove(GAMEPAD_WIN, line, 5);
    wclrtoeol(GAMEPAD_WIN); // Clear "No gamepad connected"

    // Read gamepad state
    uint32_t pressed_buttons = 0;
    float joystick_vals[4];
    if (gamepad_read(&pressed_buttons, joystick_vals) != 0) {
        mvwprintw(GAMEPAD_WIN, line, 5, "No gamepad connected!");
        // If no gamepad is connected, set the joystick values to -1 as a flag
        for (int i = 0; i < 4; i++) {
            joystick_vals[i] = -1;
        }
    }
    line++;

    // Print joysticks
    mvwprintw(GAMEPAD_WIN, line++, 1, "Joystick Positions:");
    for (int i = 0; i < 4; i++) {
        wclrtoeol(GAMEPAD_WIN); // Clear previous joystick position
        // Display joystick values iff gamepad is connected
        if (joystick_vals[i] != -1) {
            mvwprintw(GAMEPAD_WIN, line, 5, "%s = %f", joystick_names[i], joystick_vals[i]);
        }
        // Advance line pointer for the next joystick
        wmove(GAMEPAD_WIN, ++line, 5);
    }
    line += 2;

    // Print pressed buttons
    mvwprintw(GAMEPAD_WIN, line++, 1, "Pressed Buttons:");
    for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
        wclrtoeol(GAMEPAD_WIN); // Clear button name
        // Show button name if pressed
        if (pressed_buttons & (1 << i)) {
            mvwprintw(GAMEPAD_WIN, line, 5, "%s", button_names[i]);
        }
        wmove(GAMEPAD_WIN, ++line, 5);
    }

    // Draw box and refresh
    box(GAMEPAD_WIN, 0, 0);
    wrefresh(GAMEPAD_WIN);
}

/**
 * Displays a device's current state of parameters
 * Arguments:
 *    catalog: current shared memory catalog; Used to handle when device at shm_idx is invalid
 *    dev_ids: current shared memory device information; used to get device information at shm_idx
 *    shm_idx: the index of shared memory of the device to display
 */
void display_device(uint32_t catalog, dev_id_t dev_ids[MAX_DEVICES], int shm_idx) {
    const char* window_header = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Device Description~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
    const char* note = "Use the left and right arrow keys to inspect the previous or next device!";
    // Special case handling
    if (!(catalog & (1 << shm_idx))) { // Device is not connected at this shared memory index
        // Clear the window if not clear already (Happens when we disconnect a device while we're inspecting it)
        if (!DEVICE_WIN_IS_BLANK) {
            // Clear the entire window, but put back the header and the borders
            wclear(DEVICE_WIN);
            mvwprintw(DEVICE_WIN, 1, 1, window_header);
            mvwprintw(DEVICE_WIN, DEVICE_HEIGHT - 2, 1, note);
            box(DEVICE_WIN, 0, 0);
            wrefresh(DEVICE_WIN);
            DEVICE_WIN_IS_BLANK = 1; // Flag that the device window is cleared
        }
        // Return because there is no data to display for a disconnected device
        return;
    } else if (DEVICE_WIN_IS_BLANK) { // Window was previously clear (i.e Switching from no device to a connected device)
        // Put back title and box
        mvwprintw(DEVICE_WIN, 1, 1, window_header);
        mvwprintw(DEVICE_WIN, DEVICE_HEIGHT - 2, 1, note);
        box(DEVICE_WIN, 0, 0);
        wrefresh(DEVICE_WIN);
        // Note that we will display data, so DEVICE_WIN will no longer be "empty"
        DEVICE_WIN_IS_BLANK = 0;
    } else { // Switching from one connected device to another
        // log_printf(DEBUG, "Displaying connected device; DEVICE_WIN_IS_BLANK = 0");
    }

    int line = 3; // Our pointer to the current line/row we're writing to
    const int table_header_line = line + 1; // The line to display the table column names

    // Print current device identifiers
    device_t* device = get_device(dev_ids[shm_idx].type);
    if (device == NULL) {
        log_printf(DEBUG, "device == NULL");
    }
    wclrtoeol(DEVICE_WIN); // Clear previous string
    mvwprintw(DEVICE_WIN, line++, 1, "dev_ix = %d: name = %s, type = %d, year = %d, uid = %llu", shm_idx, device->name, dev_ids[shm_idx].type,  dev_ids[shm_idx].year,  dev_ids[shm_idx].uid);
    line += 2;

    // Print all command values and data values
    param_val_t command_vals[MAX_PARAMS];
    param_val_t data_vals[MAX_PARAMS];
    uint32_t cmd_map_all_devs[MAX_DEVICES + 1];
    get_cmd_map(cmd_map_all_devs);
    uint32_t cmd_map = cmd_map_all_devs[shm_idx + 1]; // We care about only the specified device
    device_read(shm_idx, EXECUTOR, COMMAND, ~0, command_vals);
    device_read(shm_idx, EXECUTOR, DATA, ~0, data_vals);
    int display_cmd_val; // Flag for whether we should display the command value for a parameter
    for (int i = 0; i < device->num_params; i++) {
        wclrtoeol(DEVICE_WIN); // Clear previous parameter entry
        // For each parameter, print the index and its name
        mvwprintw(DEVICE_WIN, line, PARAM_IDX_COL, "%d", i);
        mvwprintw(DEVICE_WIN, line, PARAM_NAME_COL, "%s", device->params[i].name);
        // Determine whether we need to display the command value
        if (device->params[i].write == 0) { // Read-only parameter
            mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "RD_ONLY");
            display_cmd_val = 0;
        } else if ((cmd_map & (1 << i)) == 0) { // No command to this parameter
            mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "NONE");
            display_cmd_val = 0;
        } else { // There is a command for the write-able parameter
            display_cmd_val = 1;
        }
        // Display values according to the parameter's type
        switch (device->params[i].type) {
            case INT:
                if (display_cmd_val) {
                    mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "%d", command_vals[i].p_i);
                }
                mvwprintw(DEVICE_WIN, line, DATA_VAL_COL, "%d", data_vals[i].p_i);
                break;
            case FLOAT:
                if (display_cmd_val) {
                    mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "%f", command_vals[i].p_f);
                }
                mvwprintw(DEVICE_WIN, line, DATA_VAL_COL, "%f", data_vals[i].p_f);
                break;
            case BOOL:
                if (display_cmd_val) {
                    mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "%s", command_vals[i].p_b ? "TRUE" : "FALSE");
                }
                mvwprintw(DEVICE_WIN, line, DATA_VAL_COL, "%s", data_vals[i].p_b ? "TRUE" : "FALSE");
                break;
        }
        // Advance our line to handle the next parameter
        wmove(DEVICE_WIN, ++line, 5);
    }
    // Clear non-existent parameters
    for (int i = device->num_params; i < MAX_PARAMS; i++) {
        wclrtoeol(DEVICE_WIN);
        wmove(DEVICE_WIN, ++line, 5);
    }

    // Draw table with schema (param_idx (int), name (str), command (variable), data (variable))
    mvwprintw(DEVICE_WIN, table_header_line, PARAM_IDX_COL, "Param Idx");
    mvwprintw(DEVICE_WIN, table_header_line, PARAM_NAME_COL, "Parameter Name");
    mvwprintw(DEVICE_WIN, table_header_line, COMMAND_VAL_COL, "Command");
    mvwprintw(DEVICE_WIN, table_header_line, DATA_VAL_COL, "Data");
    mvwhline(DEVICE_WIN, table_header_line + 1, PARAM_IDX_COL, 0, DATA_VAL_COL + VALUE_WIDTH - 6); // Horizontal line
    // Vertical Lines
    mvwvline(DEVICE_WIN, table_header_line, PARAM_NAME_COL - 1, 0, MAX_PARAMS + 2);
    mvwvline(DEVICE_WIN, table_header_line, COMMAND_VAL_COL - 1, 0, MAX_PARAMS + 2);
    mvwvline(DEVICE_WIN, table_header_line, DATA_VAL_COL - 1, 0, MAX_PARAMS + 2);

    // Box and refresh
    mvwprintw(DEVICE_WIN, DEVICE_HEIGHT - 2, 1, note);
    box(DEVICE_WIN, 0, 0);
    wrefresh(DEVICE_WIN);
}

/**
 * Displays the parameter subscriptions for each connected device by NET_HANDLER and EXECUTOR.
 * Arguments:
 *    catalog: current shared memory catalog; Used to display subscriptions for only connected devices
 */
void display_subscriptions(uint32_t catalog) {
    // Get shm subscriptions
    uint32_t sub_map[MAX_DEVICES + 1] = {0}; // Initialize subs to 0
    get_sub_requests(sub_map, SHM); // SHM indicates we want subs from both NET_HANDLER and EXECUTOR

    // Note: We won't display sub_map[0] because it is cleared when dev handler polls for subscriptions (i.e. It will almost always look 0)

    // Print parameter subscriptions (one bitmap for each device)
    int line = 3;
    mvwprintw(SUB_WIN, line++, 1, "Subscribed parameters for each device:");
    for (int i = 0; i < MAX_DEVICES; i++) {
        wclrtoeol(SUB_WIN);
        // Show subscription iff device is connected
        if (catalog & (1 << i)) {
            mvwprintw(SUB_WIN, line, 5, "Device %02d: 0x%08X", i, sub_map[i + 1]);
        }
        wmove(SUB_WIN, ++line, 5);
    }

    // Box and refresh
    box(SUB_WIN, 0, 0);
    wrefresh(SUB_WIN);
}

// ********************************** MAIN ********************************** //
void sigint_handler(int signum) {
    stop_shm();
    exit(0);
}

int main(int argc, char** argv) {
    // signal handler to clean up shm
    signal(SIGINT, sigint_handler);
    logger_init(TEST);

    // Start shm
    start_shm();
    sleep(1); // ALlow shm to initialize

    // Start UI
    initscr();

    // Inititalize ncurses windows
    init_windows();

    // Init variables for gamepad and shm data
    char** joystick_names = get_joystick_names();
    char** button_names = get_button_names();
    uint32_t catalog = 0; // Shared memory catalog (bitmap of connected devices)
    dev_id_t dev_ids[MAX_DEVICES]; // Device identifying info on each shm-connected device

    // Turn on keyboard input for DEVICE_WIN
    keypad(DEVICE_WIN, 1);
    nodelay(DEVICE_WIN, 1); // Make getch() non-blocking (return ERR if no input)

    // Holds the current device selection
    int device_selection = 0;

    // Update the UI continually (based on refresh rate)
    while (1) {
        // Detect arrow key inputs to increment/decrement device_selection
        switch (wgetch(DEVICE_WIN)) {
            case KEY_LEFT:
                // Move selection to next lowest connected device or 0
                if (device_selection > 0) {
                    device_selection--;
                }
                break;
            case KEY_RIGHT:
                // Move selection to next lowest connected device or MAX_DEVICES - 1
                if (device_selection < MAX_DEVICES - 1) {
                    device_selection++;
                }
                break;
            default:
                break;
        }

        // Get newest shm data
        get_catalog(&catalog);
        get_device_identifiers(dev_ids);

        // Update each window
        display_robot_desc();
        display_gamepad_state(joystick_names, button_names);
        display_device(catalog, dev_ids, device_selection);
        display_subscriptions(catalog);

        // Throttle refresh rate
        usleep(100000 / FPS);
        mvprintw(0, 0, "Catalog: 0x%08X; Selected Device: %02d", catalog, device_selection);
        refresh();
    }

    // End UI on key press
    getch();
    stop_shm();
    endwin();
    return 0;
}
