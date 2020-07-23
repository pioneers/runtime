UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

DEPENDENCIES=../test.* ../client/* ../../logger/* ../../net_handler/* ../../runtime_util/* ../../shm_wrapper/*
SRC_FILES=../client/*.c ../test.c ../../net_handler/pbc_gen/*.c ../../net_handler/net_util.c ../../runtime_util/*.c ../../shm_wrapper/shm_wrapper.c ../../logger/logger.c

all: test1
	
test1: $(DEPENDENCIES) test1.c
	gcc test1.c $(SRC_FILES) -o $@ $(LIBS)