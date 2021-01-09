LIBS := -pthread -lrt -Wall -lprotobuf-c -Wno-format # Ignore formatting warnings (ex: %llu vs %lu)

CC := gcc

SRC_FILES := $(wildcard client/*.c test.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c)
PBC_FILES := $(wildcard ../net_handler/pbc_gen/*.c)

TEST_FILES := $(shell ls integration/*.c | awk -F'.' '{ print $$1 }')

.PHONY: all clean cli

all: $(TEST_FILES) cli

clean:
	rm -f  $(shell find -type f ! -name "*.*" ! -name "Makefile")
	rm -f  $(shell find client/virtual_devices -type f ! -name "*.*")

################################## CLI TARGETS #################################

cli: net_handler_cli shm_cli executor_cli dev_handler_cli keyboard_interface devices

net_handler_cli: cli/net_handler_cli.* client/net_handler_client.* ../net_handler/net_util.* \
	../runtime_util/runtime_util.* ../logger/logger.c $(PBC_FILES)
	$(CC) $^ -o $@ $(LIBS)

shm_cli: cli/shm_cli.c client/shm_client.* ../shm_wrapper/shm_wrapper.* \
	../runtime_util/runtime_util.* ../logger/logger.c
	$(CC) $^ -o $@ $(LIBS)

executor_cli: cli/executor_cli.* client/executor_client.*
	$(CC) $^ -o $@ $(LIBS)

dev_handler_cli: client/dev_handler_client.* cli/dev_handler_cli.*
	$(CC) $^ -o $@

keyboard_interface: cli/keyboard_interface.c ../net_handler/*.* client/net_handler_client.* cli/net_handler_cli.c
	gcc cli/keyboard_interface.c client/net_handler_client.c ../net_handler/net_util.c \
	../net_handler/pbc_gen/*.c ../runtime_util/runtime_util.c ../logger/logger.c -o $@ $(LIBS)

################################ VIRTUAL DEVICES ###############################

VIRTUAL_DEVICE_FILES := client/virtual_devices/virtual_device_util.* ../dev_handler/message.* ../runtime_util/runtime_util.* ../logger/logger.c

VIRTUAL_DEVICES := $(shell ls client/virtual_devices/*Device.c | awk -F'.' '{ print $$1 }')

devices: $(VIRTUAL_DEVICES)
	echo "virtual devices made" > devices

client/virtual_devices/GeneralTestDevice: client/virtual_devices/GeneralTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

client/virtual_devices/SimpleTestDevice: client/virtual_devices/SimpleTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

client/virtual_devices/UnresponsiveTestDevice: client/virtual_devices/UnresponsiveTestDevice.c
	$(CC) $^ -o $@

client/virtual_devices/ForeignTestDevice: client/virtual_devices/ForeignTestDevice.c
	$(CC) $^ -o $@

client/virtual_devices/UnstableTestDevice: client/virtual_devices/UnstableTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

client/virtual_devices/TimeTestDevice: client/virtual_devices/TimeTestDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

client/virtual_devices/SoundDevice: client/virtual_devices/SoundDevice.c $(VIRTUAL_DEVICE_FILES)
	$(CC) $^ -o $@

##################################### TESTS ####################################

integration/%: $(SRC_FILES) integration/%.c
	$(CC) $^ -o $@ $(LIBS)
