#include "filter.h"

/* Gamestates:
 *   poison ivy: Motor velocities reversed for 10 seconds
 *   dehydraiton: Motor velocities stopped for 10 seconds
 *   hypothermia: Motor velocities slowed until specified otherwise
 */

// Microseconds between each time we poll shared memory for the current game state
#define GAMESTATE_POLL_INTERVAL (0.5 * 1000000)
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

typedef struct gamestate {
    uint8_t hypothermia;
    uint8_t poison_ivy;
    uint8_t dehydration;
} gamestate_t;

// A thread function that polls shared memory for gamestate updates
// Sets POISON_IVY and DEHYDRATION to INACTIVE after 10 seconds of ACTIVE
static void* gamestate_handler(void* args) {
    gamestate_t* gamestate = (gamestate_t*) args;
    uint64_t hypothermia_start;  // The timestamp of when hypothermia started; 0 if inactive
    uint64_t poison_ivy_start;   // The timestamp of when poison ivy started; 0 if inactive
    uint64_t dehydration_start;  // The timestamp of when dehydration started; 0 if inactive
    uint64_t curr_time;          // The current timestamp
    // Poll the current gamestate
    while (1) {
        curr_time = millis();
        gamestate->hypothermia = (robot_desc_read(HYPOTHERMIA) == ACTIVE) ? 1 : 0;

        if (!gamestate->poison_ivy && robot_desc_read(POISON_IVY) == ACTIVE) {  // Poison ivy started
            gamestate->poison_ivy = 1;
            poison_ivy_start = curr_time;
        } else if (gamestate->poison_ivy && (curr_time - poison_ivy_start > DEBUFF_DURATION)) {  // End poison ivy
            gamestate->poison_ivy = 0;
            poison_ivy_start = 0;
            robot_desc_write(POISON_IVY, INACTIVE);
        }

        if (!gamestate->dehydration && robot_desc_read(DEHYDRATION) == ACTIVE) {  // Dehydration started
            gamestate->dehydration = 1;
            dehydration_start = curr_time;
        } else if (gamestate->dehydration && (curr_time - gamestate->dehydration_start > DEBUFF_DURATION)) {  // End dehydration
            gamestate->dehydration = 0;
            dehydration_start = 0;
            robot_desc_write(DEHYDRATION, INACTIVE);
        }
        // Throttle the gamestate polling
        usleep(GAMESTATE_POLL_INTERVAL);
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
    static pthread_t gamestate_handler_tid = 0;
    static gamestate_t gamestate = {0};
    // Spawn thread if it doesn't already exist
    if (gamestate_handler_tid == 0) {
        pthread_create(&gamestate_handler_tid, NULL, gamestate_handler, (void*) &gamestate);
    }
    if (dev_type == KOALABEAR) {
        // Bound velocity to [-1.0, 1.0]
        bound_velocity(params);
        // Implement each gamestate, modifying params as necessary
        uint64_t curr_time = millis();
        if (gamestate.hypothermia) {
            scale_velocity(params, SLOW_SCALAR);
        }
        if (gamestate.poison_ivy) {
            scale_velocity(params, -1.0);
        }
        if (gamestate.dehydration) {
            scale_velocity(params, 0);
        }
    }
    // Call shared memory wrapper function
    return device_write_uid(dev_uid, process, stream, params_to_write, params);
}
