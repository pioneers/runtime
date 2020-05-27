# Change depending on Linux or Mac. Code won't work on Windows :(
LIBFLAGS=-lrt -lpthread # Linux
# LIBFLAGS=               # Mac

test1: shm_wrapper_test1.c shm_wrapper.c
	gcc shm_wrapper_test1.c shm_wrapper.c -o test1 $(LIBFLAGS)
	
test2: shm_wrapper_test2.c shm_wrapper.c
	gcc shm_wrapper_test2.c shm_wrapper.c -o test2 $(LIBFLAGS)
