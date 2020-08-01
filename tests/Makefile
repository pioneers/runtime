UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

DEPENDENCIES=test.* client/* ../logger/* ../net_handler/* ../runtime_util/* ../shm_wrapper/*
SRC_FILES=client/*.c test.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c

TEST_FILES=$(shell ls integration/*.c | awk -F'.' '{ print $$1 }')

all: $(TEST_FILES) cli

clean:
	rm -f  $(shell find -type f ! -name "*.*" ! -name "Makefile")

cli: net_handler_cli shm_cli executor_cli

net_handler_cli: ../net_handler/* client/net_handler_client.* cli/net_handler_cli.c
	gcc cli/net_handler_cli.c client/net_handler_client.c ../net_handler/net_util.c ../runtime_util/runtime_util.c \
		../net_handler/pbc_gen/*.c ../logger/logger.c -o $@ $(LIBS)

shm_cli: ../shm_wrapper/* client/shm_client.* cli/shm_cli.c
	gcc cli/shm_cli.c client/shm_client.c ../shm_wrapper/shm_wrapper.c ../runtime_util/runtime_util.c ../logger/logger.c -o $@ $(LIBS)

executor_cli: ../executor/* client/executor_client.* cli/executor_cli.c
	gcc cli/executor_cli.c client/executor_client.c ../runtime_util/runtime_util.c ../logger/logger.c -o $@ $(LIBS)

dev_handler_cli: client/dev_handler_client.c cli/dev_handler_cli.c general_dev
	gcc client/dev_handler_client.c cli/dev_handler_cli.c -o $@

# TODO: Add target for all future virtual devices
general_dev: client/virtual_devices/general_dev.c ../dev_handler/message.c ../runtime_util/runtime_util.c ../logger/logger.c
	gcc $^ -o client/virtual_devices/GeneralTestDevice

integration/%: $(SRC_FILES)
	gcc $@.c $(SRC_FILES) -o $* $(LIBS)
