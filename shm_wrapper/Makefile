# Change depending on Linux or Mac. Code won't work on Windows :(
# LIBFLAGS=-lrt -lpthread # Linux
LIBFLAGS=               # Mac

test1: shm_wrapper_test1.c shm_wrapper.c ../logger/logger.c ../logger/logger_config.h
	gcc shm_wrapper_test1.c shm_wrapper.c ../logger/logger.c -o test1 $(LIBFLAGS)
	
test2: shm_wrapper_test2.c shm_wrapper.c ../logger/logger.c ../logger/logger_config.h
	gcc shm_wrapper_test2.c shm_wrapper.c ../logger/logger.c -o test2 $(LIBFLAGS)
