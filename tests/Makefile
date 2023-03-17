LIBS := -pthread -lrt -Wall -lprotobuf-c -Wno-format # Ignore formatting warnings (ex: %llu vs %lu)
BIN = bin

CC := gcc

SRC_FILES := $(wildcard client/*.c test.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c)
PBC_FILES := $(wildcard ../net_handler/pbc_gen/*.c)

TEST_FILES := $(shell ls integration/*.c performance/*.c | awk -F'.' '{ print $$1 }')

.PHONY: all clean cli

all: $(TEST_FILES) cli

clean:
	rm -rf $(BIN)/*

################################## CLI TARGETS #################################

cli: net_handler_cli shm_ui executor_cli dev_handler_cli devices

net_handler_cli: cli/net_handler_cli.* client/net_handler_client.* cli/keyboard_interface.* ../net_handler/net_util.* \
	../runtime_util/runtime_util.* ../logger/logger.c $(PBC_FILES)
	$(CC) $^ -o $(BIN)/$@ $(LIBS)

shm_ui: cli/shm_ui.c client/shm_client.c ../shm_wrapper/shm_wrapper.c \
	../runtime_util/runtime_util.c ../logger/logger.c
	$(CC) $^ -o $(BIN)/$@ $(LIBS) -lncurses

executor_cli: cli/executor_cli.* client/executor_client.*
	$(CC) $^ -o $(BIN)/$@ $(LIBS)

dev_handler_cli: client/dev_handler_client.* cli/dev_handler_cli.*
	$(CC) $^ -o $(BIN)/$@

################################ VIRTUAL DEVICES ###############################

VIRTUAL_DEVICE_FILES := client/virtual_devices/virtual_device_util.* ../dev_handler/message.* ../runtime_util/runtime_util.* ../logger/logger.c

VIRTUAL_DEVICES := $(shell ls client/virtual_devices/*Device.c | awk -F'.' '{ print $$1 }')

devices: folder $(VIRTUAL_DEVICES)
	echo "virtual devices made" > bin/virtual_devices/devices

folder:
	mkdir -p $(BIN)/virtual_devices

client/virtual_devices/GeneralTestDevice: client/virtual_devices/GeneralTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $(BIN)/virtual_devices/GeneralTestDevice

client/virtual_devices/SimpleTestDevice: client/virtual_devices/SimpleTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $(BIN)/virtual_devices/SimpleTestDevice

client/virtual_devices/UnresponsiveTestDevice: client/virtual_devices/UnresponsiveTestDevice.c
	$(CC) $^ -o $(BIN)/virtual_devices/UnresponsiveTestDevice

client/virtual_devices/ForeignTestDevice: client/virtual_devices/ForeignTestDevice.c
	$(CC) $^ -o $(BIN)/virtual_devices/ForeignTestDevice

client/virtual_devices/UnstableTestDevice: client/virtual_devices/UnstableTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $(BIN)/virtual_devices/UnstableTestDevice

client/virtual_devices/TimeTestDevice: client/virtual_devices/TimeTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $(BIN)/virtual_devices/TimeTestDevice

client/virtual_devices/SoundDevice: client/virtual_devices/SoundDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $(BIN)/virtual_devices/SoundDevice

##################################### TESTS ####################################

integration/%: $(SRC_FILES) integration/%.c
	mkdir -p $(BIN)/integration
	$(CC) $^ -o $(BIN)/$@ $(LIBS)

performance/%: $(SRC_FILES) performance/%.c
	mkdir -p $(BIN)/performance
	$(CC) $^ -o $(BIN)/$@ $(LIBS)
