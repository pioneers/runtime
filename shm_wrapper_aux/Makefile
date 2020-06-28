UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt
endif
ifeq ($(UNAME), Darwin)
LIBS=
endif

test1: shm_wrapper_aux_test1.c shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h ../runtime_util/runtime_util.c
	gcc shm_wrapper_aux_test1.c shm_wrapper_aux.c ../logger/logger.c ../runtime_util/runtime_util.c -o test1 $(LIBS)
	
test2: shm_wrapper_aux_test2.c shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h ../runtime_util/runtime_util.c
	gcc shm_wrapper_aux_test2.c shm_wrapper_aux.c ../logger/logger.c ../runtime_util/runtime_util.c -o test2 $(LIBS)

static: connect_static_aux.c shm_wrapper_aux.c ../logger/logger.c ../runtime_util/runtime_util.c
	gcc connect_static_aux.c shm_wrapper_aux.c ../logger/logger.c  ../runtime_util/runtime_util.c -o static $(LIBS)
