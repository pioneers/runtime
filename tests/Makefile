LIBS := -pthread -lrt -Wall -lprotobuf-c -Wno-format # Ignore formatting warnings (ex: %llu vs %lu)

CC := gcc

SRC_FILES := $(wildcard client/*.c test.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c)
OBJ_FILES := $(SRC_FILES:.c=.o)

TEST_FILES := $(shell ls integration/*.c | awk -F'.' '{ print $$1 }')

.PHONY: all clean cli devices

all: $(TEST_FILES) cli

clean:
	rm -f  $(shell find -type f ! -name "*.*" ! -name "Makefile")
	rm -f  $(shell find client/virtual_devices -type f ! -name "*.*")
	rm -f  $(shell find .. -type f -name "*.o")

%.o: %.c
	$(CC) -c -o $@ $< $(LIBS)

################################## CLI TARGETS #################################

cli: net_handler_cli shm_cli executor_cli dev_handler_cli_and_devs

PBC_FILES := $(wildcard ../net_handler/pbc_gen/*.c)

net_handler_cli: cli/net_handler_cli.o client/net_handler_client.o ../net_handler/net_util.o \
	../runtime_util/runtime_util.o $(PBC_FILES:.c=.o) ../logger/logger.o
	$(CC) $^ -o $@ $(LIBS)

shm_cli: cli/shm_cli.o client/shm_client.o ../shm_wrapper/shm_wrapper.o \
	../runtime_util/runtime_util.o ../logger/logger.o
	$(CC) $^ -o $@ $(LIBS)

executor_cli: cli/executor_cli.o client/executor_client.o ../runtime_util/runtime_util.o \
	../logger/logger.o
	$(CC) $^ -o $@ $(LIBS)

dev_handler_cli_and_devs: devices dev_handler_cli

dev_handler_cli: client/dev_handler_client.o cli/dev_handler_cli.o
	$(CC) $^ -o $@

################################ VIRTUAL DEVICES ###############################

devices: general_dev simple_dev unresponsive_dev foreign_dev unstable_dev time_dev

VIRTUAL_DEVICE_FILES := client/virtual_devices/virtual_device_util.o ../dev_handler/message.o ../runtime_util/runtime_util.o ../logger/logger.o

general_dev: client/virtual_devices/GeneralTestDevice

client/virtual_devices/GeneralTestDevice: client/virtual_devices/general_dev.o $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

simple_dev: client/virtual_devices/SimpleTestDevice

client/virtual_devices/SimpleTestDevice: client/virtual_devices/simple_dev.o $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

unresponsive_dev: client/virtual_devices/UnresponsiveTestDevice

client/virtual_devices/UnresponsiveTestDevice: client/virtual_devices/unresponsive_dev.o
	$(CC) $^ -o $@

foreign_dev: client/virtual_devices/ForeignTestDevice

client/virtual_devices/ForeignTestDevice: client/virtual_devices/foreign_dev.o
	$(CC) $^ -o $@

unstable_dev: client/virtual_devices/UnstableTestDevice

client/virtual_devices/UnstableTestDevice: client/virtual_devices/unstable_dev.o $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

time_dev: client/virtual_devices/TimeTestDevice

client/virtual_devices/TimeTestDevice: client/virtual_devices/time_dev.o $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

##################################### TESTS ####################################

integration/%: $(OBJ_FILES) integration/%.o
	$(CC) $^ -o $@ $(LIBS)
