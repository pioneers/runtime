#include "../test.h"

/**
 * Tests that when the Robot is set to IDLE, all game states are set to INACTIVE.
 */

int main() {
    start_test("IDLE Clears Game States", "", NO_REGEX);

    robot_desc_field_t game_states[] = {POISON_IVY, DEHYDRATION, HYPOTHERMIA};
    size_t num_game_states = sizeof(game_states) / sizeof(game_states[0]);

    // Start run mode that uses game states
    send_run_mode(SHEPHERD, AUTO);

    // Start game states
    for (size_t i = 0; i < num_game_states; i++) {
        send_game_state(game_states[i]);
    }
    sleep(1);

    // Verify that the game states actually are in effect
    for (size_t i = 0; i < num_game_states; i++) {
        check_game_state(game_states[i], ACTIVE);
    }

    // Set to IDLE, which should stop all game states
    send_run_mode(SHEPHERD, IDLE);
    sleep(1);

    // Verify that game states are inactive
    for (size_t i = 0; i < num_game_states; i++) {
        check_game_state(game_states[i], INACTIVE);
    }

    return 0;
}
