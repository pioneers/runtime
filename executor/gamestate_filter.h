#ifndef FILTER_H
#define FILTER_H

/**
 * Serves as a filter/wrapper around shared memory wrapper.
 * This interface is to be used only by the Student API's Robot.set_value()
 *    Instead of calling shm wrapper's device_write() or device_write_uid(),
 *    it should call the corresponding functions in this file.
 *
 * The purpose is so that we can modify student calls to Robot.set_value()
 * if necessary to enforce certain game states.
 * As a result, the implementation of functions in this file is expected to
 * change each year.
 *
 * Example:
 *    1) Shepherd broadcasts to Runtime that robot motors should be reversed
 *       while in "poison ivy" state.
 *    2) The poison ivy state is recorded in shared memory.
 *    3) While poison ivy is active,
 *           All calls to Robot.set_value(KoalaBear, "velocity", value)
 *           will flip the sign of VALUE before being written to shared memory.
 */

#include <stdbool.h>
#include "../logger/logger.h"
#include "../runtime_util/runtime_util.h"
#include "../shm_wrapper/shm_wrapper.h"

/**
 * Starts the gamestate handler thread so that gamestates are deactivated
 * after the respective debuff duration passes.
 * All game states are deactivated also when run mode is set to IDLE
 * If the game state handler thread is already active for the process calling this,
 * this won't spawn a new thread (no-op).
 */
void start_gamestate_handler_thread();

/**
 * A wrapper function to device_write_uid that modifies the input params
 * based on the current active game states.
 */
int filter_device_write_uid(uint8_t dev_type, uint64_t dev_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t* params);

#endif
