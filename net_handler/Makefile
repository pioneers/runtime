# list of libraries that net_handler needs to compile
LIBS=-pthread -lrt -Wall -lprotobuf-c

######################################  protobuf defs

PROTO = protos
PBC_OBJ = $(BUILD)/pbc_obj
PBC_GEN = pbc_gen
BUILD_DIR = $(PBC_GEN) $(PBC_OBJ) # add PBC_GEN and PBC_OBJ to list of build directories

PROTOS = $(shell cd $(PROTO) && ls *.proto && cd ..) # list all the protos
PBC_SRCS = $(patsubst %.proto,$(PBC_GEN)/%.pb-c.c, $(PROTOS)) # list of all generated source files by doing a path substitution
PBC_OBJS = $(patsubst $(PBC_GEN)/%.c,$(PBC_OBJ)/%.o, $(PBC_SRCS)) # list of all generated object files by doing a path substitution

INCLUDE = -I$(PBC_GEN)
CFLAGS += $(shell pkg-config --cflags libprotobuf-c) # append more CFLAGS for compiling protobuf to existing CFLAGS

#####################################

# list of source files that the target (net_handler) depends on, relative to this folder
SRCS = net_handler.c net_handler_message.c net_util.c connection.c ../executor/gamestate_filter.c \
	 ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c

# specify the target (executable we want to make)
TARGET = net_handler

.PHONY: all clean $(TARGET) protos

all: protos $(TARGET)

# include top-level Makefile
include ../Makefile_defs

# resolve phony target "protos" as make all protobuf object files
protos: $(PBC_OBJS)

# resolve phony target "net_handler" as make ../bin/net_handler
$(TARGET): $(BIN)/$(TARGET)

# rule to compile net_handler
$(BIN)/$(TARGET): $(OBJS) $(PBC_OBJS) | $(BIN)
	$(CC) $(OBJS) $(PBC_OBJS) -o $@ $(LIBS)

# makes the generated protobuf-c object files in the $(PBC_OBJ) folder
$(PBC_OBJ)/%.o: $(PBC_GEN)/%.c | $(PBC_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

# makes the generated protobuf-c files in the $(PBC_GEN) folder
$(PBC_GEN)/%.pb-c.c: $(PROTO)/%.proto | $(PBC_GEN)
	cd $(PROTO) && protoc-c --c_out ../$(PBC_GEN) $*.proto

# remove build artifacts
clean:
	rm -f $(OBJS)
	rm -rf $(PBC_OBJ)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -f $(BIN)/$(TARGET)

# also remove generated protobufs
very-clean: clean
	rm -rf $(PBC_GEN)

-include $(patsubst %.o,%.d,$(OBJS))
-include $(patsubst %.o,%.d,$(PBC_OBJS))
