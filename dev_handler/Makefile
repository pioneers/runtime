LIBFLAGS=-pthread -lrt -Wall

dev_handler: dev_handler.c message.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c
	gcc $^ -o dev_handler $(LIBFLAGS)

clean:
	rm -f ./dev_handler
    