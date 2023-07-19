# list of libraries that network_switch needs to compile
LIBS=-pthread -lrt -Wall

# list of source files that the target (network_switch) depends on, relative to this folder
SRCS = network_switch.c ../runtime_util/runtime_util.c ../logger/logger.c ../shm_wrapper/shm_wrapper.c

# specify the target (executable we want to make)
TARGET = network_switch

.PHONY: all clean $(TARGET)

all: $(TARGET)

# include top-level Makefile
include ../Makefile_defs

# resolve phony target "network_switch" as ../bin/network_switch
$(TARGET): $(BIN)/$(TARGET)

# rule to compile network_switch
$(BIN)/$(TARGET): $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $@ $(LIBS)

# remove build artifacts
clean:
	rm -f $(OBJS)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -f $(BIN)/$(TARGET)

-include $(patsubst %.o,%.d,$(OBJS))
