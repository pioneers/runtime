CC=gcc
LIBFLAGS=-pthread -lrt

DEV_HANDLER_FILES=message.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c

# Run dev_handler
dev: dev_handler.c $(DEV_HANDLER_FILES)
	$(CC) $^ -o dev_handler $(LIBFLAGS)
	# sudo ./dev_handler

# Run dev_handler with Valgrind
val: dev_handler.c $(DEV_HANDLER_FILES)
	$(CC) $^ -o dev_handler $(LIBFLAGS)
	valgrind ./dev_handler

# Clean files generated from FILE_DEV
clean:
	rm -f ./dev_handler
