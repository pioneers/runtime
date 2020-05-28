from libc.stdint cimport *

cdef extern from "../logger/logger_config.h":
    enum log_level:
        INFO, DEBUG, WARN, ERROR, FATAL

cdef extern from "../runtime_util/runtime_util.h":
    ctypedef enum process_t:
        DEV_HANDLER, EXECUTOR, NET_HANDLER
    ctypedef struct dev_t:
        pass
    ctypedef struct param_val_t:
        pass

cdef extern from "../logger/logger.h":
    void logger_init (process_t process)
    void logger_stop ()
    void log_runtime (log_level level, char *msg)

cdef extern from "../shm_wrapper/shm_wrapper.h":
    ctypedef enum stream_t:
        DATA, COMMAND
    void device_read (int dev_ix, process_t process, stream_t stream, uint32_t params_to_read, param_val_t *params)
    void device_write (int dev_ix, process_t process, stream_t stream, uint32_t params_to_write, param_val_t *params)

cdef extern from "../dev_handler/devices.h":
    uint16_t device_name_to_type(char* dev_name)
    uint8_t get_param_id(uint16_t dev_type, char* param_name)


