# list of libraries that dev_handler needs to compile
LIBS=-pthread -lrt -Wall

# list of source files that the target (dev_handler) depends on, relative to this folder
SRCS = dev_handler.c dev_handler_message.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c

# specify the target (executable we want to make)
TARGET = dev_handler

.PHONY: all clean $(TARGET)

all: $(TARGET)

# include top-level Makefile
include ../Makefile_defs

# resolve phony target "dev_handler" as ../bin/dev_handler
$(TARGET): $(BIN)/$(TARGET)

# rule to compile dev_handler
$(BIN)/$(TARGET): $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $@ $(LIBS)

# remove build artifacts
clean:
	rm -f $(OBJS)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -f $(BIN)/$(TARGET)

-include $(patsubst %.o,%.d,$(OBJS))
