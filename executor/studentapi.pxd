from libc.stdint cimport *

cdef extern from "../logger/logger_config.h":
    enum log_level:
        INFO, DEBUG, WARN, ERROR, FATAL

cdef extern from "../runtime_util/runtime_util.h":
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

cdef extern from "../logger/logger.h":
    void logger_init (process_t process)
    void logger_stop ()
    void log_runtime (log_level level, char *msg)

cdef extern from "../shm_wrapper/shm_wrapper.h":
    ctypedef enum stream_t:
        DATA, COMMAND
    void shm_init(process_t process)
    void shm_stop(process_t process)
    void device_read (int dev_idx, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
    void device_write (int dev_idx, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)
    int get_device_idx_from_uid(uint64_t dev_uid)
    void print_dev_ids()
    void print_params (uint32_t params_to_print, param_val_t* params)

cdef extern from "../dev_handler/devices.h":
    int MAX_PARAMS
    device_t* get_device(uint16_t device_type)
    uint16_t device_name_to_type(char* dev_name)
    char* device_type_to_name(uint16_t dev_type)
    void all_params_for_device_type(uint16_t dev_type, char* param_names[])
    param_desc_t* get_param_desc(uint16_t dev_type, char* param_name)
    uint8_t get_param_idx(uint16_t dev_type, char* param_name)


