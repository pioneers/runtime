UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

UTILS=../runtime_util/runtime_util.c ../logger/logger.c
SRC_FILES=client/*.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c ../shm_wrapper/shm_wrapper.c $(UTILS)

all: tester.c cli

clean:
	rm -f tester.c
	rm -f net_handler_cli
	rm -f shm_cli
	rm -f executor_cli

cli: net_handler_cli shm_cli executor_cli

net_handler_cli: cli/net_handler_cli.c client/net_handler_client.c ../net_handler/pbc_gen/*.c ../net_handler/net_util.c $(UTILS)
	gcc $^ -o $@ $(LIBS)

shm_cli: cli/shm_cli.c client/shm_client.c ../shm_wrapper/shm_wrapper.c $(UTILS)
	gcc $^ -o $@ $(LIBS)

executor_cli: cli/executor_cli.c client/executor_client.c $(UTILS)
	gcc $^ -o $@ $(LIBS)

tester.c: tester.pyx $(SRC_FILES)
	# python3 setup.py build_ext -i
	python3 setup.py install --user
