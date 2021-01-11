/**
 * The UI for shared memory using ncurses, a third-party dependency (not required to run runtime)
 * This displays a live view of memory blocks in SHM by polling frequently.
 * 
 * Important blocks of shared memory each have a dedicated ncurses "window"
 * 
 * Note that messages sent to stdout will interfere with the ui.
 * Note also that if a parameter is not subscribed to, then shared memory will not have the most updated
 *    parameter data, so the data stream for that parameter will look "frozen" (This is normal)
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

#include "../../logger/logger.h"
#include "../../runtime_util/runtime_util.h"
#include "../../shm_wrapper/shm_wrapper.h"
#include "../client/shm_client.h"

// The refresh rate of the UI and how often we poll shared memory data
#define FPS 2

// ******************************** WINDOWS ********************************* //
WINDOW* ROBOT_DESC_WIN;  // Displays the robot description (clients connected, run mode, start position, gamepad connected)
WINDOW* GAMEPAD_WIN;     // Displays gamepad state (joystick values and what buttons are pressed)
WINDOW* DEVICE_WIN;      // Displays device information (id, commands, data, and subscriptions) for a single device at a time based on user input

// ************************** SIZES AND POSITIONS *************************** //
// Some windows' dimensions are defined in terms of others so that their borders align

// Enough to fit each value
#define ROBOT_DESC_HEIGHT 8
// ROBOT_DESC_WIDTH is enough to fit GAMEPAD_WIDTH (which needs to be wider than ROBOT_DESC)
#define ROBOT_DESC_WIDTH 35
#define ROBOT_DESC_START_Y 3
#define ROBOT_DESC_START_X 0

// GAMEPAD_HEIGHT is enough to fit all the joysticks and buttons
#define GAMEPAD_HEIGHT 28
#define GAMEPAD_WIDTH ROBOT_DESC_WIDTH
// Display GAMEPAD_WIN below ROBOT_DESC
#define GAMEPAD_START_Y (ROBOT_DESC_START_Y + ROBOT_DESC_HEIGHT)
#define GAMEPAD_START_X ROBOT_DESC_START_X

// DEVICE_HEIGHT should be large enough to display all the parameters,
// with extra lines for device id, the table itself, and the message about using the arrow keys
#define DEVICE_HEIGHT (MAX_PARAMS + 7)
// DEVICE_WIDTH is enough to fit the information for each parameter
#define DEVICE_WIDTH 75
#define DEVICE_START_Y 0
// Make the left border of DEVICE_WIN to the right of ROBOT_DESC_WIN and GAMEPAD_WIN
#define DEVICE_START_X (ROBOT_DESC_WIDTH + 1)

// The left-most column at which we display something in each window
// (i.e. There will be INDENT - 1 spaces between the left border and the first character)
#define INDENT 3

// Column positions in device window; Declared as const instead of #define to prevent repeated strlen() computation
// Each one is determined by taking the previous column and adding the previous column's maximum width
const int VALUE_WIDTH = strlen("123.000000") + 1;                        // String representation of float length; Used to determine column widths
const int PARAM_IDX_COL = INDENT;                                             // The column at which we display the parameter index
const int PARAM_NAME_COL = PARAM_IDX_COL + strlen("00") + 1;          // The column at which we display the parameter name (Idx column is 2 digits wide)
const int NET_SUB_COL = PARAM_NAME_COL + strlen("increasing_even") + 1;  // The column at which we display whether the parameter is subbed by net handler
const int EXE_SUB_COL = NET_SUB_COL + strlen("NET") + 1;                 // The column at which we display whether the parameter is subbed by executor
const int COMMAND_VAL_COL = EXE_SUB_COL + strlen("EXE") + 1;             // The column at which we display the command stream
const int DATA_VAL_COL = COMMAND_VAL_COL + VALUE_WIDTH;                  // The column at which we display the data stream

// **************************** MISC GLOBAL VARS **************************** //

/* Bool of whether the DEVICE_WIN is currently blank.
 * This is used to properly handle switching between connected and disconnected devices.
 * if viewing disconnected device:
 *    if !DEVICE_WIN_IS_BLANK
 *        clear the entire window
 *        DEVICE_WIN_IS_BLANK = 1
 *    return
 * if DEVICE_WIN_IS_BLANK
 *    // We want to display information, but the window is blank!
 *    Rebuild the window (with headers, notes, etc)
 *    DEVICE_WIN_IS_BLANK = 0
 */
int DEVICE_WIN_IS_BLANK = 0;

// The header of DEVICE_WIN; Useful for clearing and redrawing DEVICE_WIN
#define DEVICE_WIN_HEADER "~~~~~~~~~~~~~~~~~~~~~~~~~~~~DEVICE INFORMATION~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

// The note displayed at the bottom of DEVICE_WIN
const char* NOTE = "Use the left and right arrow keys to inspect the previous or next device!";

// The number of possible "devices" displayed
// (The normal MAX_DEVICES plus the custom data block, which is presented as a "device" with parameters)
#define DEVICE_WRAP (MAX_DEVICES + 1)

// ******************************** NCURSES ********************************* //

// Initializes ncurses windows
void init_windows() {
    // Robot description
    ROBOT_DESC_WIN = newwin(ROBOT_DESC_HEIGHT, ROBOT_DESC_WIDTH, ROBOT_DESC_START_Y, ROBOT_DESC_START_X);
    wattron(ROBOT_DESC_WIN, A_BOLD);
    mvwprintw(ROBOT_DESC_WIN, 1, 1, "~~~~~~~~ROBOT DESCRIPTION~~~~~~~~");
    wattroff(ROBOT_DESC_WIN, A_BOLD);
    // Gamepad
    GAMEPAD_WIN = newwin(GAMEPAD_HEIGHT, GAMEPAD_WIDTH, GAMEPAD_START_Y, GAMEPAD_START_X);
    wattron(GAMEPAD_WIN, A_BOLD);
    mvwprintw(GAMEPAD_WIN, 1, 1, "~~~~~~~~~~GAMEPAD STATE~~~~~~~~~~");
    wattroff(GAMEPAD_WIN, A_BOLD);
    // Device info (device data)
    DEVICE_WIN = newwin(DEVICE_HEIGHT, DEVICE_WIDTH, DEVICE_START_Y, DEVICE_START_X);
    wattron(DEVICE_WIN, A_BOLD);
    mvwprintw(DEVICE_WIN, 1, 1, DEVICE_WIN_HEADER);
    wattroff(DEVICE_WIN, A_BOLD);
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
    int line = 2;
    wmove(ROBOT_DESC_WIN, line, 0);
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, INDENT, "RUN_MODE\t= %s", (run_mode == IDLE) ? "IDLE" : (run_mode == AUTO ? "AUTO" : (run_mode == TELEOP ? "TELEOP" : "CHALLENGE")));
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, INDENT, "DAWN\t\t= %s", (dawn_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, INDENT, "SHEPHERD\t= %s", (shepherd_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, INDENT, "GAMEPAD\t= %s", (gamepad_connection == CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    wclrtoeol(ROBOT_DESC_WIN);
    mvwprintw(ROBOT_DESC_WIN, line++, INDENT, "START_POS\t= %s", (start_pos == LEFT) ? "LEFT" : "RIGHT");

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
    int line = 2;  // Note that this is just a row number; Must use wmove() to actually move the cursor to this line
    wmove(GAMEPAD_WIN, line, 0);
    wclrtoeol(GAMEPAD_WIN);  // Clear "No gamepad connected"

    // Read gamepad state if gamepad is connected
    uint32_t pressed_buttons = 0;
    float joystick_vals[4] = {0};
    int gamepad_connected = (robot_desc_read(GAMEPAD) == CONNECTED);
    if (gamepad_connected) {
        gamepad_read(&pressed_buttons, joystick_vals);
    } else {
        mvwprintw(GAMEPAD_WIN, line, INDENT, "No gamepad connected!");
    }
    wmove(GAMEPAD_WIN, ++line, 0);

    // Print joysticks
    wclrtoeol(GAMEPAD_WIN);
    if (gamepad_connected) {
        mvwprintw(GAMEPAD_WIN, line, 1, "Joystick Positions:");
    }
    wmove(GAMEPAD_WIN, ++line, 0);
    for (int i = 0; i < 4; i++) {
        // Clear previous entry
        wclrtoeol(GAMEPAD_WIN);
        // Display joystick values (This will be 0 if gamepad is not connected)
        if (gamepad_connected) {
            mvwprintw(GAMEPAD_WIN, line, INDENT, "%s\t= %.4f", joystick_names[i], joystick_vals[i]);
        }
        // Advance line pointer for the next joystick
        wmove(GAMEPAD_WIN, ++line, 0);
    }

    // Move to buttons section
    line++;
    wmove(GAMEPAD_WIN, line, 0);

    // Print pressed buttons
    wclrtoeol(GAMEPAD_WIN);
    if (gamepad_connected) {
        mvwprintw(GAMEPAD_WIN, line, 1, "Pressed Buttons:");
    }
    wmove(GAMEPAD_WIN, ++line, 0);
    for (int i = 0; i < NUM_GAMEPAD_BUTTONS; i++) {
        // Move to button row and clear previous entry
        wclrtoeol(GAMEPAD_WIN);
        // Show button name; If pressed, make it bold. Else, dim
        if (gamepad_connected) {
            if (pressed_buttons & (1 << i)) {
                wattron(GAMEPAD_WIN, A_BOLD);
            } else {
                wattron(GAMEPAD_WIN, A_DIM);
            }
            mvwprintw(GAMEPAD_WIN, line, INDENT, "%s", button_names[i]);
            wattroff(GAMEPAD_WIN, A_BOLD | A_DIM); // Turn off any text attributes
        }
        // Move to next button
        wmove(GAMEPAD_WIN, ++line, 0);
    }

    // Draw box and refresh
    box(GAMEPAD_WIN, 0, 0);
    wrefresh(GAMEPAD_WIN);
}

/**
 * Displays a device's current state of parameters in a formatted table.
 * NET (NET_HANDLER) and EXE (EXECUTOR) refers to whether the parameter is subscribed by the process.
 * Param Idx (int) | Name (str) | NET (bool) | EXE (bool) | Command (var) | Data (var)
 * 
 * Arguments:
 *    catalog: current shared memory catalog; Used to handle when device at shm_idx is invalid
 *    dev_ids: current shared memory device information; used to get device information at shm_idx
 *    shm_idx: the index of shared memory of the device to display
 *             set to MAX_DEVICES if displaying the custom data block is desired
 */
void display_device(uint32_t catalog, dev_id_t dev_ids[MAX_DEVICES], int shm_idx) {
    // Special case handling
    const int show_custom_data = (shm_idx == MAX_DEVICES);
    if (!show_custom_data && !(catalog & (1 << shm_idx))) {  // Device is not connected at this shared memory index
        // Clear the window if not clear already (Happens when we disconnect a device while we're inspecting it)
        if (!DEVICE_WIN_IS_BLANK) {
            // Clear the entire window, but put back the header and the borders
            wclear(DEVICE_WIN);
            wattron(DEVICE_WIN, A_BOLD);
            mvwprintw(DEVICE_WIN, 1, 1, DEVICE_WIN_HEADER);
            wattroff(DEVICE_WIN, A_BOLD);
            mvwprintw(DEVICE_WIN, 2, 1, "This device was disconnected!");
            mvwprintw(DEVICE_WIN, DEVICE_HEIGHT - 2, 1, NOTE);
            box(DEVICE_WIN, 0, 0);
            wrefresh(DEVICE_WIN);
            DEVICE_WIN_IS_BLANK = 1;  // Flag that the device window is cleared
        }
        // Return because there is no data to display for a disconnected device
        return;
    } else if (DEVICE_WIN_IS_BLANK) {  // Window was previously clear (i.e Switching from no device to a connected device)
        // Put back title and box
        mvwprintw(DEVICE_WIN, 1, 1, DEVICE_WIN_HEADER);
        mvwprintw(DEVICE_WIN, DEVICE_HEIGHT - 2, 1, NOTE);
        box(DEVICE_WIN, 0, 0);
        wrefresh(DEVICE_WIN);
        // Note that we will display data, so DEVICE_WIN will no longer be "empty"
        DEVICE_WIN_IS_BLANK = 0;
    } else {
        // Switching from one connected device to another
    }

    int line = 2;  // Our pointer to the current line/row we're writing to
    wmove(DEVICE_WIN, line, 0);
    const int table_header_line = line + 1;  // The line to display the table column names

    // Print current device's identifiers
    device_t* device = NULL;
    wclrtoeol(DEVICE_WIN);  // Clear previous string
    if (show_custom_data) {
        mvwprintw(DEVICE_WIN, line++, INDENT, "Custom Data:");
    } else {
        device = get_device(dev_ids[shm_idx].type);
        if (device == NULL) {  // This should never happen if the handling above is correct
            log_printf(ERROR, "device == NULL");
        }
        mvwprintw(DEVICE_WIN, line++, 1, "dev_ix = %d: name = %s, type = %d, year = %d, uid = %llu", shm_idx, device->name, dev_ids[shm_idx].type, dev_ids[shm_idx].year, dev_ids[shm_idx].uid);
    }
    // Move to the first parameter (Skip the next two lines because we'll display the table headers with the horizontal line)
    line += 2;
    wmove(DEVICE_WIN, line, 0);

    // Number of parameters to display for this specific device
    uint8_t num_params = 0;

    // Init arrays to hold shm data
    uint32_t sub_map_net_handler_all_devs[MAX_DEVICES + 1] = {0};  // Initialize subs to 0
    uint32_t sub_map_executor_all_devs[MAX_DEVICES + 1] = {0};     // Initialize subs to 0
    uint32_t cmd_map_all_devs[MAX_DEVICES + 1];
    param_val_t command_vals[MAX_PARAMS];
    param_val_t data_vals[MAX_PARAMS];

    // Init variables to hold custom data information
    char custom_param_names[UCHAR_MAX][64];
    param_type_t custom_param_types[UCHAR_MAX];
    param_val_t custom_param_values[UCHAR_MAX];  // We're assuming here that the number of logged params is at most MAX_PARAMS

    // Get shm subscriptions
    if (show_custom_data) {
        log_data_read(&num_params, custom_param_names, custom_param_types, custom_param_values);
    } else {
        num_params = device->num_params;
        // Get shm subscriptions, command values, and data values
        get_sub_requests(sub_map_net_handler_all_devs, NET_HANDLER);  // Get subscriptions made by net handler
        get_sub_requests(sub_map_executor_all_devs, EXECUTOR);        // Get subscriptions made by executor
        get_cmd_map(cmd_map_all_devs);
        device_read(shm_idx, SHM, COMMAND, ~0, command_vals);
        device_read(shm_idx, SHM, DATA, ~0, data_vals);
    }

    // We care about only the specified device (this is just for the sake of brevity)
    uint32_t sub_map_net_handler = sub_map_net_handler_all_devs[shm_idx + 1];
    uint32_t sub_map_executor = sub_map_executor_all_devs[shm_idx + 1];
    uint32_t cmd_map = cmd_map_all_devs[shm_idx + 1];

    // Each iteration prints out a row for each parameter
    int display_cmd_val;  // Flag for whether we should display the command value for a parameter
    for (int i = 0; i < num_params; i++) {
        wclrtoeol(DEVICE_WIN);  // Clear previous parameter entry

        // Print the index and its name
        mvwprintw(DEVICE_WIN, line, PARAM_IDX_COL, "%d", i);
        mvwprintw(DEVICE_WIN, line, PARAM_NAME_COL, "%s", show_custom_data ? custom_param_names[i] : device->params[i].name);

        // Print out subscriptions ("X" iff subbed)
        // Note that subscriptions are irrelevant for custom data
        //   Runtime will *always* send custom data to net handler
        //   Executor has nothing to do with the custom data
        if (show_custom_data || sub_map_net_handler & (1 << i)) {
            mvwprintw(DEVICE_WIN, line, NET_SUB_COL + 1, "X");
        }
        if (!show_custom_data && sub_map_executor & (1 << i)) {
            mvwprintw(DEVICE_WIN, line, EXE_SUB_COL + 1, "X");
        }

        // Determine whether we need to display the command value
        if (show_custom_data) {
            mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "N/A");
            display_cmd_val = 0;
        } else if (device->params[i].write == 0) {  // Read-only parameter
            mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "RD_ONLY");
            display_cmd_val = 0;
        } else if ((cmd_map & (1 << i)) == 0) {  // No command to this parameter
            mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "NONE");
            display_cmd_val = 0;
        } else {  // There is a command for the write-able parameter
            display_cmd_val = 1;
        }

        // Display values according to the parameter's type
        param_type_t param_type = (show_custom_data) ? custom_param_types[i] : device->params[i].type;
        param_val_t param_val = (show_custom_data) ? custom_param_values[i] : data_vals[i];
        switch (param_type) {
            case INT:
                if (display_cmd_val) {
                    mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "%d", command_vals[i].p_i);
                }
                mvwprintw(DEVICE_WIN, line, DATA_VAL_COL, "%d", param_val.p_i);
                break;
            case FLOAT:
                if (display_cmd_val) {
                    mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "%f", command_vals[i].p_f);
                }
                mvwprintw(DEVICE_WIN, line, DATA_VAL_COL, "%f", param_val.p_f);
                break;
            case BOOL:
                // Display the string representation of the boolean
                if (display_cmd_val) {
                    mvwprintw(DEVICE_WIN, line, COMMAND_VAL_COL, "%s", command_vals[i].p_b ? "TRUE" : "FALSE");
                }
                mvwprintw(DEVICE_WIN, line, DATA_VAL_COL, "%s", param_val.p_b ? "TRUE" : "FALSE");
                break;
        }
        // Advance our line to handle the next parameter
        wmove(DEVICE_WIN, ++line, 0);
    }
    // Clear non-existent parameters
    for (int i = num_params; i < MAX_PARAMS; i++) {
        wclrtoeol(DEVICE_WIN);
        wmove(DEVICE_WIN, ++line, 0);
    }

    // Draw table
    mvwprintw(DEVICE_WIN, table_header_line, PARAM_IDX_COL, "#");
    mvwprintw(DEVICE_WIN, table_header_line, PARAM_NAME_COL, "Parameter Name");
    mvwprintw(DEVICE_WIN, table_header_line, NET_SUB_COL, "NET");
    mvwprintw(DEVICE_WIN, table_header_line, EXE_SUB_COL, "EXE");
    mvwprintw(DEVICE_WIN, table_header_line, COMMAND_VAL_COL, "Command");
    mvwprintw(DEVICE_WIN, table_header_line, DATA_VAL_COL, "Data");
    mvwhline(DEVICE_WIN, table_header_line + 1, PARAM_IDX_COL, 0, DEVICE_WIDTH - PARAM_IDX_COL - INDENT);  // Horizontal line
    // Vertical Lines
    mvwvline(DEVICE_WIN, table_header_line, PARAM_NAME_COL - 1, 0, MAX_PARAMS + 2);
    mvwvline(DEVICE_WIN, table_header_line, NET_SUB_COL - 1, 0, MAX_PARAMS + 2);
    mvwvline(DEVICE_WIN, table_header_line, EXE_SUB_COL - 1, 0, MAX_PARAMS + 2);
    mvwvline(DEVICE_WIN, table_header_line, COMMAND_VAL_COL - 1, 0, MAX_PARAMS + 2);
    mvwvline(DEVICE_WIN, table_header_line, DATA_VAL_COL - 1, 0, MAX_PARAMS + 2);

    // Box and refresh
    mvwprintw(DEVICE_WIN, DEVICE_HEIGHT - 2, 1, NOTE);
    box(DEVICE_WIN, 0, 0);
    wrefresh(DEVICE_WIN);
}

// ********************************** MAIN ********************************** //

// Sending SIGINT to the process will stop shared memory
void clean_up(int signum) {
    stop_shm();
    endwin();
    exit(0);
}

int main(int argc, char** argv) {
    // signal handler to clean up shm
    signal(SIGINT, clean_up);
    logger_init(TEST);

    // Start shm
    start_shm();
    sleep(1);  // Allow shm to initialize

    // Start UI
    initscr();

    // Inititalize ncurses windows
    init_windows();

    // Init variables for gamepad and shm data
    char** joystick_names = get_joystick_names();
    char** button_names = get_button_names();
    uint32_t catalog = 0;           // Shared memory catalog (bitmap of connected devices)
    dev_id_t dev_ids[MAX_DEVICES];  // Device identifying info on each shm-connected device

    // Turn on keyboard input for DEVICE_WIN
    keypad(DEVICE_WIN, 1);
    nodelay(DEVICE_WIN, 1);  // Make getch() non-blocking (return ERR if no input)

    // Holds the current device selection
    int device_selection = MAX_DEVICES;  // Initialize to custom data, which is always present

    // Update the UI continually (based on refresh rate)
    while (1) {
        // Get newest shm data
        get_catalog(&catalog);
        get_device_identifiers(dev_ids);

        // Detect arrow key inputs to increase/decrease device_selection
        int direction = 0;
        switch (wgetch(DEVICE_WIN)) {
            case KEY_LEFT:
                direction = -1;
                break;
            case KEY_RIGHT:
                direction = 1;
                break;
            default:
                break;
        }

        // Device selection is updated, with wrapping within [0, MAX_DEVICES] (inclusive)
        // The device selection will move to the closest connected device in that direction, in
        // The device at index MAX_DEVICES is the custom data block, which is visible to Dawn as another "device"
        if (direction != 0) {  // User wants to change device being displayed
            // Increment/decrement device selection first, then
            // While the device selection is not a connected device and not the custom data,
            // move it in the selected direction
            do {
                device_selection += direction;
                device_selection = (device_selection + DEVICE_WRAP) % DEVICE_WRAP;
            } while (device_selection != MAX_DEVICES && !(catalog & (1 << device_selection)));
        }

        // Update each window
        display_robot_desc();
        display_gamepad_state(joystick_names, button_names);
        display_device(catalog, dev_ids, device_selection);

        // Throttle refresh rate
        usleep(100000 / FPS);

        // Display catalog and current device selection
        int line = 0;
        mvprintw(line++, 1, "Shared Memory Dashboard");
        mvprintw(line++, 1, "Catalog:\t  0x%08X", catalog);
        mvprintw(line++, 1, "Selected Device: %02d", device_selection);

        refresh();
    }

    // Properly clean up
    stop_shm();
    endwin();
    return 0;
}
