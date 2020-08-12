LIBFLAGS=-pthread -lrt

dev_handler: dev_handler.c message.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c
	gcc $^ -o dev_handler $(LIBFLAGS)

# Clean files generated from FILE_DEV
clean:
	rm -f ./dev_handler
