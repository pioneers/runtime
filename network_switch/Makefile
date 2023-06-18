LIBFLAGS=-pthread -lrt -Wall
BIN=../bin


network_switch: network_switch.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c
	gcc $^ -o $(BIN)/network_switch

clean:
	rm -f $(BIN)/network_switch
	rm -f network_switch
    