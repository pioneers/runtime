UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

all: net_handler_cli shm_cli

net_handler_cli: ../../net_handler/* ../client/net_handler_client.* net_handler_cli.c
	cd ../../net_handler && make net_handler
	gcc net_handler_cli.c ../client/net_handler_client.c ../../net_handler/net_util.c ../../runtime_util/runtime_util.c \
		../../net_handler/pbc_gen/*.c ../../logger/logger.c -o $@ $(LIBS)

shm_cli: ../../shm_wrapper/* ../client/shm_client.* shm_cli.c
	cd ../../shm_wrapper && make shm
	gcc shm_cli.c ../client/shm_client.c ../../shm_wrapper/shm_wrapper.c ../../runtime_util/runtime_util.c ../../logger/logger.c -o $@ $(LIBS)
	
executor_cli: ../../executor/* ../client/executor_client.* executor_cli.c
	cd ../../executor && make
	gcc executor_cli.c ../client/executor_client.c ../../runtime_util/runtime_util.c ../../logger/logger.c -o $@ $(LIBS)
