from libc.stdint cimport *

cdef extern from "../logger/logger_config.h":
    enum log_level:
        INFO, DEBUG, WARN, ERROR, FATAL


cdef extern from "../runtime_util/runtime_util.h":
    int NUM_GAMEPAD_BUTTONS
    ctypedef enum process_t:
        EXECUTOR, API
    ctypedef struct device_t:
        pass
    ctypedef struct param_val_t:
        int p_i
        float p_f
        uint8_t p_b
    ctypedef struct param_desc_t:
        char* name
        char* type
    char** get_button_names()
    char** get_joystick_names()


cdef extern from "../logger/logger.h" nogil:
    void logger_init (process_t process)
    void logger_stop ()
    void log_runtime (log_level level, char *msg)


cdef extern from "../shm_wrapper/shm_wrapper.h" nogil:
    ctypedef enum stream_t:
        DATA, COMMAND
    void shm_init(process_t process)
    void shm_stop(process_t process)
    void device_read_uid(uint64_t device_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
    void device_write_uid(uint64_t device_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
    void print_dev_ids()
    void print_params (uint32_t params_to_print, param_val_t* params)


cdef extern from "../shm_wrapper_aux/shm_wrapper_aux.h" nogil:
    void shm_aux_init (process_t process)
    void shm_aux_stop (process_t process)
    void gamepad_read (process_t process, uint32_t *pressed_buttons, float *joystick_vals);


cdef extern from "../dev_handler/devices.h" nogil:
    int MAX_PARAMS
    param_desc_t* get_param_desc(uint16_t dev_type, char* param_name)
    uint8_t get_param_idx(uint16_t dev_type, char* param_name)


cdef extern from "executor.h":
    void run_robot_function(char* func_name)
    int is_function_running(char* func_name)
