###################################### compiler flags

# libraries needed to compile the executables
LIBS=-pthread -lrt -Wall -lprotobuf-c -lncurses -Wno-format
# specify the compiler we use (gcc)
CC = gcc
# add CFLAGS to auto-generate dependency files
CFLAGS = -MMD -MP

######################################  protobuf defs

# getting paths to protobuf because needed for compilation but this Makefile will NOT compile them (compile them in net_handler!)
PROTO = ../net_handler/protos
PBC_OBJ = $(BUILD)/pbc_obj
PBC_GEN = ../net_handler/pbc_gen

PROTOS = $(shell cd $(PROTO) && ls *.proto && cd ..) # list all the protos
PBC_SRCS = $(patsubst %.proto,$(PBC_GEN)/%.pb-c.c, $(PROTOS)) # list of all generated source files by doing a path substitution
PBC_OBJS = $(patsubst $(PBC_GEN)/%.c,$(PBC_OBJ)/%.o, $(PBC_SRCS)) # list of all generated object files by doing a path substitution

# add generated protobuf headers to list of folders to include in compilation
INCLUDE = -I$(PBC_GEN)
CFLAGS += $(shell pkg-config --cflags libprotobuf-c) # append more CFLAGS for compiling protobuf

#################################### name directories and lists of directories

# tests will have its own bin directory (tests/bin), but will use the global build directory for object files (../build)
BIN = bin
BUILD = ../build
OBJ = $(BUILD)/obj
THIS_DIR = tests

COMPONENT_DIRS = dev_handler net_handler executor network_switch shm_wrapper runtime_util logger tests # list of runtime component directories
OBJ_SUBDIR = $(foreach dir,$(COMPONENT_DIRS),$(OBJ)/$(dir)) # list of subfolders in $(OBJ) for runtime object files
INCLUDE += $(foreach dir,$(COMPONENT_DIRS),-I../$(dir)) # list of folders to include in compilation commands

# start making the lists of build and bin directories
SUBDIR = cli client client/virtual_devices performance integration
BUILD_DIR = $(foreach dir,$(SUBDIR),$(OBJ)/$(THIS_DIR)/$(dir)) $(OBJ) $(OBJ_SUBDIR)
BIN_DIR = $(BIN)

##################################### make lists of files

# list of source files that various executables are dependent on
UTIL_SRCS = ../runtime_util/runtime_util.c ../logger/logger.c # everybody uses these
NET_HANDLER_CLI_SRCS = cli/net_handler_cli.c client/net_handler_client.c cli/keyboard_interface.c ../net_handler/net_util.c $(UTIL_SRCS)
SHM_UI_SRCS = cli/shm_ui.c client/shm_client.c ../shm_wrapper/shm_wrapper.c $(UTIL_SRCS)
EXECUTOR_CLI_SRCS = cli/executor_cli.c client/executor_client.c $(UTIL_SRCS)
DEV_HANDLER_CLI_SRCS = client/dev_handler_client.c cli/dev_handler_cli.c $(UTIL_SRCS)

# list of source files that each virtual device has as a dependency
VIRTUAL_DEV_SRCS = client/virtual_devices/virtual_device_util.c ../dev_handler/dev_handler_message.c $(UTIL_SRCS)

# list of source files that each test has as a dependency
TESTS_SRCS = test.c $(wildcard client/*.c) ../net_handler/net_util.c ../shm_wrapper/shm_wrapper.c $(UTIL_SRCS)

# list of relative paths to virtual device source files from this directory (e.g. client/virtual_devices/GeneralTestDevice.c)
VIRTUAL_DEVICES = $(wildcard client/virtual_devices/*Device.c)

# list of relative paths to test source files from this directory (e.g. integration/tc_150_1.c)
TESTS = $(wildcard integration/* performance/*)

# generate lists of object files from the lists of source files above using path substituion
# i.e. if a source file is client/net_handler_client.c, we substitute to obtain ../build/obj/tests/client/net_handler_client.o)
NET_HANDLER_CLI_OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(NET_HANDLER_CLI_SRCS))
SHM_UI_OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(SHM_UI_SRCS))
EXECUTOR_CLI_OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(EXECUTOR_CLI_SRCS))
DEV_HANDLER_CLI_OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(DEV_HANDLER_CLI_SRCS))
TEST_OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(TESTS_SRCS))
VIRTUAL_DEV_OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(VIRTUAL_DEV_SRCS))

# specify directories for virtual devices and tests executables
VIRTUAL_DEV_EXE_DIR = $(BIN)/virtual_devices
TESTS_EXE_DIR = $(BIN)/integration $(BIN)/performance
BIN_DIR += $(VIRTUAL_DEV_EXE_DIR) $(TESTS_EXE_DIR)

# specify targets (arguments you can give to make i.e. "make net_handler_cli" or "make tc_150_1" or "make GeneralTestDevice")
CLI_TARGET = net_handler_cli executor_cli dev_handler_cli shm_ui
TEST_TARGET = $(patsubst %.c,%,$(TESTS)) # e.g. "integration/tc_150_1"
TEST_TARGET_SHORT = $(foreach test_target,$(TEST_TARGET),$(shell basename $(test_target))) # e.g. "tc_150_1"
VIRTUAL_DEV_TARGET = $(patsubst client/virtual_devices/%.c,%,$(VIRTUAL_DEVICES)) # e.g. "GeneralTestDevice"

# generate lists of executable names using path substitution
CLI_EXE = $(patsubst %,$(BIN)/%,$(CLI_TARGET)) # e.g. bin/net_handler_cli
TEST_EXE = $(patsubst %,$(BIN)/%,$(TEST_TARGET)) # e.g. bin/integration/tc_150_1
VIRTUAL_DEV_EXE = $(patsubst %,$(BIN)/virtual_devices/%,$(VIRTUAL_DEV_TARGET)) # e.g. bin/virtual_devices/GeneralTestDevice

# combine all source files and associated object files with each other into a list for .c -> .o rule
SRCS = $(NET_HANDLER_CLI_SRCS) $(SHM_UI_SRCS) $(EXECUTOR_CLI_SRCS) $(DEV_HANDLER_CLI_SRCS) \
	 $(VIRTUAL_DEV_SRCS) $(TESTS_SRCS) $(VIRTUAL_DEVICES) $(TESTS)
OBJS = $(patsubst %.c,$(OBJ)/$(THIS_DIR)/%.o,$(SRCS)) # generate list of all object files relative to this Makefile

#################################### RULES BEGIN HERE ##################################

# we need this to evaluate any rule that uses the name of the target in its list of dependencies
# i.e. any rule that contains double dollar signs "$$"
.SECONDEXPANSION:

# declare all of these targets as phony because these are not file names that exist
.PHONY: all cli tests devices clean help $(CLI_TARGET) $(TEST_TARGET) $(TEST_TARGET_SHORT) $(VIRTUAL_DEV_TARGET)

# targets that should be made when just "make" is run in this directory
all: cli tests devices

#################################### rules to compile cli

# resolve phony target "cli" to list of all cli executables
cli: $(CLI_EXE)

# resolve phony cli target to its corresponding executable, e.g. "make net_handler_cli" -> "make bin/net_handler_cli"
$(CLI_TARGET): $(BIN)/$$@

# below are four rules to compile the CLIs (too lazy to combine them into one...)
# net_handler_cli is special because it needs $(PBC_OBJS)
$(BIN)/net_handler_cli: $(NET_HANDLER_CLI_OBJS) $(PBC_OBJS) | $(BIN)
	$(CC) $^ -o $@ $(LIBS)

$(BIN)/executor_cli: $(EXECUTOR_CLI_OBJS) | $(BIN)
	$(CC) $^ -o $@ $(LIBS)

$(BIN)/dev_handler_cli: $(DEV_HANDLER_CLI_OBJS) | $(BIN)
	$(CC) $^ -o $@ $(LIBS)

$(BIN)/shm_ui: $(SHM_UI_OBJS) | $(BIN)
	$(CC) $^ -o $@ $(LIBS)

################################## rules to compile virtual device

# resolve phony target "devices" to list of all virtual device executables
devices: $(VIRTUAL_DEV_EXE)

# resolve phony virtual device target to its corresponding executable,
# e.g. "make GeneralTestDevice" -> "make bin/virtual_devices/GeneralTestDevice"
$(VIRTUAL_DEV_TARGET): $$(patsubst %,$(BIN)/virtual_devices/%,$$@)

# rule to compile a virtual device executable
$(VIRTUAL_DEV_EXE): $$(patsubst $(BIN)/%,$(OBJ)/$(THIS_DIR)/client/%.o,$$@) $(VIRTUAL_DEV_OBJS) | $(VIRTUAL_DEV_EXE_DIR)
	$(CC) $^ -o $@ $(LIBS)

################################# rules to compile test

# resolve phony target "tests" to list of all test executables
tests: $(TEST_EXE)

# resolve phony test target to its corresponding executable,
# e.g. "make integration/tc_150_1" -> "make bin/integration/tc_150_1"
$(TEST_TARGET): $$(patsubst %,$(BIN)/%,$$@)

# resolve phony test target short to its corresponding executable
# e.g. "make tc_150_1" -> "make bin/integration/tc_150_1"
$(TEST_TARGET_SHORT): $$(filter %$$@,$(TEST_EXE))

# rule to compile a test executable
$(TEST_EXE): $$(patsubst $(BIN)/%,$(OBJ)/$(THIS_DIR)/%.o,$$@) $(TEST_OBJS) $(PBC_OBJS) | $(TESTS_EXE_DIR)
	$(CC) $^ -o $@ $(LIBS)

################################ general rule for compiling a list of source files to object files in the $(OBJ) directory

# e.g. to make "../build/obj/tests/client/net_handler_client.o", it depends on
# "client/net_handler_client.c" and the build directories must be created, then run the command
# "$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@" to compile, which translates, in this case, to the following:
# [gcc][-MMD -MP ...][-I../runtime_util -I../net_handler/pbc_gen .....] -c [client/net_handler_client.c]
#            -o [../build/obj/tests/client/net_handler_client.o]
$(OBJ)/$(THIS_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

################################ general rule for making a build or bin directory

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

############################### remove build artifacts

clean:
	rm -f $(OBJS)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -rf $(BIN)

############################## help text

help:
	@printf "\nYou can make CLIs, virtual devices, and tests using this Makefile\n\n"
	@printf "\tCLIs:\n"
	@printf "\t\tTo make all CLIs, run \"make cli\"\n"
	@printf "\t\tTo make a specific CLI, you can run, for example, \"make net_handler_cli\"\n"
	@printf "\t\tThe resulting executables will be in tests/bin/<*_cli>, e.g. \"tests/bin/net_handler_cli\"\n"
	@printf "\tVirtual Devices:\n"
	@printf "\t\tTo make all virtual devices, run \"make devices\"\n"
	@printf "\t\tTo make a specific virtual device, you can run, for example, \"make GeneralTestDevice\"\n"
	@printf "\t\tThe resulting executables will be in tests/bin/virtual_devices/*, e.g. \"tests/bin/virtual_devices/GeneralTestDevice\"\n"
	@printf "\tTests:\n"
	@printf "\t\tTo make all tests, run \"make tests\"\n"
	@printf "\t\tTo make a specific test, you can run, for example \"make tc_150_1\" OR \"make integration/tc_150_1\"\n"
	@printf "\t\tThe resulting executables will be in tests/bin/integration/* or tests/bin/performance/*,\n"
	@printf "\t\t\te.g. \"tests/bin/integration/tc_150_1\" or \"tests/bin/performacne/tc_71_10\"\n"
	@printf "\tAll:\n"
	@printf "\t\tMake everything by running either \"make all\" or simply just \"make\"\n"
	@printf "\tClean:\n"
	@printf "\t\tRemove all build artificats (tests/bin, all object files, etc.) by running \"make clean\"\n\n\n"

############################## include generated dependency files

-include $(patsubst %.o,%.d,$(OBJS))

