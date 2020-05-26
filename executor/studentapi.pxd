from libc cimport stdint

cdef extern from "../logger/logger_config.h":
    enum log_level:
        INFO, DEBUG, WARN, ERROR, FATAL

cdef extern from "../runtime_util.h":
    ctypedef enum process_t:
        DEV_HANDLER, EXECUTOR, NET_HANDLER

cdef extern from "../logger/logger.h":
    void logger_init (process_t process)
    void logger_stop ()
    void log_runtime (log_level level, char *msg)

cdef extern from "executor.h":
    pass

# cdef extern from "../device_handler/device_handler.c":
#     pass
