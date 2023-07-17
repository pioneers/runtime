BIN = bin
BUILD = build
OBJ = $(BUILD)/obj

COMPONENT_DIRS = dev_handler net_handler executor network_switch shm_wrapper runtime_util logger
INCLUDE_FLAGS = $(foreach dir,$(COMPONENT_DIRS),-I$(dir)) # list of folders to include in compilation commands

BUILD_DIRS = $(BIN) $(BUILD) $(OBJ)

OBJS_ALL = 

$(BUILD_DIRS):
	mkdir -p $(BUILD_DIRS)
	

	
clean:
	rm -rf $(BUILD_DIRS)