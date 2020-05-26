LIBFLAGS=-lrt -lpthread # Change depending on Linux or Mac. See README for details

test_read: shm_mmap_read.c  shm_wrapper.c
	gcc shm_mmap_read.c shm_wrapper.c -o test_read $(LIBFLAGS)

test_write: shm_mmap_write.c shm_wrapper.c
	gcc shm_mmap_write.c shm_wrapper.c -o test_write $(LIBFLAGS)

test1: shm_wrapper_test1.c shm_wrapper.c
	gcc shm_wrapper_test1.c shm_wrapper.c -o test1 $(LIBFLAGS)
	
test2: shm_wrapper_test2.c shm_wrapper.c
	gcc shm_wrapper_test2.c shm_wrapper.c -o test2 $(LIBFLAGS)
