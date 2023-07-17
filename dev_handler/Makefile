LIBS=-pthread -lrt -Wall

# include top-level Makefile
include ../Makefile

SRCS = dev_handler.c message.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c
HEADERS = message.h ../logger/logger.h ../runtime_util/runtime_util.h ../shm_wrapper/shm_wrapper.h

OBJS = $(foreach file,$(SRCS),../$(OBJ)/$(shell basename $(file)).o) # list of all object files for dev handler

dev_handler: $(SRCS) $(HEADERS) | $(BIN)
	gcc $(OBJS) -o $(BIN)/$@ $(LIBS)
    
