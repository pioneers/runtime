#include "gamestate_filter.h"

/* Gamestates:
 *   poison ivy: Motor velocities reversed for 10 seconds
 *   dehydraiton: Motor velocities stopped for 10 seconds
 *   hypothermia: Motor velocities slowed until specified otherwise
 */

// Microseconds between each time we poll shared memory for the current game state
#define GAMESTATE_POLL_INTERVAL (0.5 * 1e6)
// Duration of POISON_IVY and DEHYDRATION in milliseconds
#define DEBUFF_DURATION 10000
// How much to slow motor velocities when HYPOTHERMIA is ACTIVE
#define SLOW_SCALAR 0.2

// Some "hardcoded" values to decrease overhead
// device_name_to_type("KoalaBear")
#define KOALABEAR 6
// Parameter indices of the KoalaBear
#define IDX_VELOCITY_A 0
#define IDX_VELOCITY_B 8

/**
 * A thread function that acts as a timer on gamestates and blocks forever.
 * Sets POISON_IVY and DEHYDRATION to INACTIVE after DEBUFF_DURATION milliseconds of ACTIVE.
 * Deactivates all game states when run mode is set to IDLE.
 * Arguments:
 *    Unused
 * Returns:
 *    NULL
 */
static void* gamestate_handler(void* args) {
    uint64_t poison_ivy_start = 0;   // The timestamp of when poison ivy started; 0 if inactive
    uint64_t dehydration_start = 0;  // The timestamp of when dehydration started; 0 if inactive
    uint64_t curr_time = 0;          // The current timestamp

    // Poll the current gamestate
    while (1) {
        curr_time = millis();

        if (robot_desc_read(RUN_MODE) == IDLE) {
            robot_desc_write(HYPOTHERMIA, INACTIVE);
            robot_desc_write(POISON_IVY, INACTIVE);
            robot_desc_write(DEHYDRATION, INACTIVE);
            poison_ivy_start = 0;
            dehydration_start = 0;
        } else {  // Game state are in play; Deactivate when debuff duration passes
            // Update poison ivy
            if (!poison_ivy_start && robot_desc_read(POISON_IVY) == ACTIVE) {  // Poison ivy had just started since the last poll
                poison_ivy_start = curr_time;
            } else if (poison_ivy_start && (curr_time - poison_ivy_start > DEBUFF_DURATION)) {  // Poison ivy had just ended since the last poll
                poison_ivy_start = 0;
                robot_desc_write(POISON_IVY, INACTIVE);
            }

            // Update dehydration
            if (!dehydration_start && robot_desc_read(DEHYDRATION) == ACTIVE) {  // Dehydration had just started since the last poll
                dehydration_start = curr_time;
            } else if (dehydration_start && (curr_time - dehydration_start > DEBUFF_DURATION)) {  // Dehydration had just started since the last poll
                dehydration_start = 0;
                robot_desc_write(DEHYDRATION, INACTIVE);
            }
        }

        // Throttle the gamestate polling
        usleep(GAMESTATE_POLL_INTERVAL);
    }
    return NULL;
}

void start_gamestate_handler_thread() {
    static pthread_t gamestate_handler_tid = 0;
    if (gamestate_handler_tid == 0) {  // There should be only one thread monitoring game states.
        pthread_create(&gamestate_handler_tid, NULL, gamestate_handler, NULL);
    }
}

// Scales the KoalaBear's velocities by SCALAR
// Assumes PARAMS belong to a KoalaBear
static void scale_velocity(param_val_t* params, float scalar) {
    params[IDX_VELOCITY_A].p_f *= scalar;
    params[IDX_VELOCITY_B].p_f *= scalar;
}

// Bound the KoalaBear's velocities to [-1.0, 1.0]
// Assumes PARAMS belong to a KoalaBear
static void bound_velocity(param_val_t* params) {
    // Handle velocity A
    if (params[IDX_VELOCITY_A].p_f > 1.0) {
        params[IDX_VELOCITY_A].p_f = 1.0;
    } else if (params[IDX_VELOCITY_A].p_f < -1.0) {
        params[IDX_VELOCITY_A].p_f = -1.0;
    }
    // Handle velocity B
    if (params[IDX_VELOCITY_B].p_f > 1.0) {
        params[IDX_VELOCITY_B].p_f = 1.0;
    } else if (params[IDX_VELOCITY_B].p_f < -1.0) {
        params[IDX_VELOCITY_B].p_f = -1.0;
    }
}

int filter_device_write_uid(uint8_t dev_type, uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params) {
    // Spring 2021: Only KoalaBear is affected by game states
    if (dev_type == KOALABEAR) {
        // Bound velocity to [-1.0, 1.0]
        bound_velocity(params);

        // Implement each gamestate, modifying params as necessary
        if (robot_desc_read(HYPOTHERMIA) == ACTIVE) {
            scale_velocity(params, SLOW_SCALAR);
        }
        if (robot_desc_read(POISON_IVY) == ACTIVE) {
            scale_velocity(params, -1.0);
        }
        if (robot_desc_read(DEHYDRATION) == ACTIVE) {
            scale_velocity(params, 0);
        }
    }
    // Call the actual shared memory wrapper function with the (possibly modified) values
    return device_write_uid(dev_uid, process, stream, params_to_write, params);
}
