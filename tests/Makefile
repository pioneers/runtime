LIBS=-pthread -lrt -Wall -lprotobuf-c -Wno-format # Ignore formatting warnings (ex: %llu vs %lu)

DEPENDENCIES=test.* client/* ../logger/* ../net_handler/* ../runtime_util/* ../shm_wrapper/*
SRC_FILES=client/*.c test.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c

TEST_FILES=$(shell ls integration/*.c | awk -F'.' '{ print $$1 }')

all: $(TEST_FILES) cli

clean:
	rm -f  $(shell find -type f ! -name "*.*" ! -name "Makefile")
	rm -f  $(shell find client/virtual_devices -type f ! -name "*.*")

################################## CLI TARGETS #################################

cli: net_handler_cli shm_cli executor_cli dev_handler_cli

net_handler_cli: ../net_handler/*.* client/net_handler_client.* cli/net_handler_cli.c
	gcc cli/net_handler_cli.c client/net_handler_client.c ../net_handler/net_util.c ../runtime_util/runtime_util.c \
		../net_handler/pbc_gen/*.c ../logger/logger.c -o $@ $(LIBS)

shm_cli: ../shm_wrapper/*.* client/shm_client.* cli/shm_cli.*
	gcc cli/shm_cli.c client/shm_client.c ../shm_wrapper/shm_wrapper.c ../runtime_util/runtime_util.c ../logger/logger.c -o $@ $(LIBS)

executor_cli: ../executor/*.* client/executor_client.* cli/executor_cli.*
	gcc cli/executor_cli.c client/executor_client.c ../runtime_util/runtime_util.c ../logger/logger.c -o $@ $(LIBS)

dev_handler_cli: ../dev_handler/*.* client/dev_handler_client.* cli/dev_handler_cli.* devices
	gcc client/dev_handler_client.c cli/dev_handler_cli.c -o $@

################################ VIRTUAL DEVICES ###############################

devices: general_dev simple_dev unresponsive_dev foreign_dev unstable_dev time_dev

VIRTUAL_DEVICE_FILES=client/virtual_devices/virtual_device_util.c ../dev_handler/message.c ../runtime_util/runtime_util.c ../logger/logger.c

general_dev: client/virtual_devices/general_dev.c $(VIRTUAL_DEVICE_FILES)
	gcc $^ -o client/virtual_devices/GeneralTestDevice

simple_dev: client/virtual_devices/simple_dev.c $(VIRTUAL_DEVICE_FILES)
	gcc $^ -o client/virtual_devices/SimpleTestDevice

unresponsive_dev: client/virtual_devices/unresponsive_dev.c
	gcc $^ -o client/virtual_devices/UnresponsiveTestDevice

foreign_dev: client/virtual_devices/foreign_dev.c
	gcc $^ -o client/virtual_devices/ForeignTestDevice

unstable_dev: client/virtual_devices/unstable_dev.c $(VIRTUAL_DEVICE_FILES)
	gcc $^ -o client/virtual_devices/UnstableTestDevice

time_dev: client/virtual_devices/time_dev.c $(VIRTUAL_DEVICE_FILES)
	gcc $^ -o client/virtual_devices/TimeTestDevice

##################################### TESTS ####################################

integration/%: $(SRC_FILES)
	gcc $@.c $(SRC_FILES) -o $* $(LIBS)
