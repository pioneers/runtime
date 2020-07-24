UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

DEPENDENCIES=../test.* ../client/* ../../logger/* ../../net_handler/* ../../runtime_util/* ../../shm_wrapper/*
SRC_FILES=../client/*.c ../test.c ../../net_handler/pbc_gen/*.c ../../net_handler/net_util.c ../../runtime_util/runtime_util.c ../../shm_wrapper/shm_wrapper.c ../../logger/logger.c

all: test_sanity

test_sanity: $(DEPENDENCIES) test_sanity.c
	gcc test_sanity.c $(SRC_FILES) -o $@ $(LIBS)

test_constant_print: $(DEPENDENCIES) test_constant_print.c
	gcc test_constant_print.c $(SRC_FILES) -o $@ $(LIBS)
