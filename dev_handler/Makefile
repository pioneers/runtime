LIBFLAGS=-pthread -lrt -Wall
BIN=../bin

dev_handler: dev_handler.c message.c message.h ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c
	gcc $^ -o $(BIN)/dev_handler $(LIBFLAGS)

network_switch: network_switch.c
	gcc $^ -o $(BIN)/network_switch

clean:
	rm -f $(BIN)/dev_handler
	rm -f dev_handler
    
