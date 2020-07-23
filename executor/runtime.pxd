from libc.stdint cimport *

cdef extern from "../runtime_util/runtime_util.h":
    int MAX_DEVICES
    int MAX_PARAMS
    int NUM_GAMEPAD_BUTTONS
    ctypedef enum process_t:
        EXECUTOR
    ctypedef struct device_t:
        char* name
        uint8_t type
        uint8_t num_params
        param_desc_t* params
    ctypedef union param_val_t:
        int16_t p_i
        float p_f
        uint8_t p_b
    ctypedef enum param_type_t:
        INT, FLOAT, BOOL
    ctypedef struct param_desc_t:
        char* name
        param_type_t type
    ctypedef enum robot_desc_field_t:
        GAMEPAD, START_POS, RUN_MODE
    ctypedef enum robot_desc_val_t:
        CONNECTED, DISCONNECTED, LEFT, RIGHT, AUTO, TELEOP
    char** get_button_names() nogil
    char** get_joystick_names() nogil
    param_desc_t* get_param_desc(uint8_t dev_type, char* param_name) nogil
    int8_t get_param_idx(uint8_t dev_type, char* param_name) nogil
    device_t* get_device(uint8_t dev_type) nogil


cdef extern from "../logger/logger.h":
    void logger_init (process_t process)
    void logger_stop ()
    void log_printf (log_level level, char *format, ...)
    enum log_level:
        DEBUG, INFO, WARN, PYTHON, ERROR, FATAL


cdef extern from "../shm_wrapper/shm_wrapper.h" nogil:
    ctypedef enum stream_t:
        DATA, COMMAND
    void shm_init()
    void shm_stop()
    int device_read_uid(uint64_t device_uid, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
    int device_write_uid(uint64_t device_uid, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
    int gamepad_read (uint32_t *pressed_buttons, float *joystick_vals)
    robot_desc_val_t robot_desc_read (robot_desc_field_t field)
    int place_sub_request (uint64_t dev_uid, process_t process, uint32_t params_to_sub)
