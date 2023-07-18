# list of libraries that dev_handler needs to compile
LIBS=-pthread -lrt -Wall

# list of source files that the target (dev_handler) depends on, relative to this folder
SRCS = dev_handler.c dev_handler_message.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c

include ../Makefile

all: dev_handler

dev_handler: $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $(BIN)/$@ $(LIBS)

clean:
	rm -f $(OBJS)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -f $(BIN)/dev_handler

-include $(patsubst %.o,%.d,$(OBJS))
