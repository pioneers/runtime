UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

all: net_handler_cli

net_handler_cli: ../../net_handler/* ../client/net_handler_client.* net_handler_cli.c
	cd ../../net_handler && make net_handler
	gcc net_handler_cli.c ../client/net_handler_client.c ../../net_handler/net_util.c ../../runtime_util/runtime_util.c \
		../../net_handler/pbc_gen/*.c ../../logger/logger.c -o $@ $(LIBS)
