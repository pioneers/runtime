from libc.stdint cimport *

cdef extern from "../runtime_util/runtime_util.h":
    int NUM_GAMEPAD_BUTTONS
    ctypedef enum process_t:
        EXECUTOR
    ctypedef struct device_t:
        char* name
    ctypedef union param_val_t:
        int p_i
        float p_f
        uint8_t p_b
    ctypedef struct param_desc_t:
        char* name
        char* type
    char** get_button_names() nogil
    char** get_joystick_names() nogil
    int MAX_PARAMS
    param_desc_t* get_param_desc(uint16_t dev_type, char* param_name) nogil
    int8_t get_param_idx(uint16_t dev_type, char* param_name) nogil


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
    int gamepad_read (uint32_t *pressed_buttons, float *joystick_vals);
