# Change depending on Linux or Mac. Code won't work on Windows :(
# LIBFLAGS=-lrt -lpthread # Linux
LIBFLAGS=               # Mac

test1: shm_wrapper_aux_test1.c shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h
	gcc shm_wrapper_aux_test1.c shm_wrapper_aux.c ../logger/logger.c -o test1 $(LIBFLAGS)
	
test2: shm_wrapper_aux_test2.c shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h
	gcc shm_wrapper_aux_test2.c shm_wrapper_aux.c ../logger/logger.c -o test2 $(LIBFLAGS)
