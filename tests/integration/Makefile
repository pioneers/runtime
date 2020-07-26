UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall -lprotobuf-c
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall -lprotobuf-c
endif

SRC_FILES=../client/*.c ../test.c ../../net_handler/pbc_gen/*.c ../../net_handler/net_util.c ../../runtime_util/runtime_util.c ../../shm_wrapper/shm_wrapper.c ../../logger/logger.c

TEST_SRC=$(wildcard *.c)
TEST_EXE=$(patsubst %.c,%,$(TEST_SRC))

all: $(TEST_EXE)

% : %.c
	gcc $@.c $(SRC_FILES) -o $@ $(LIBS)

clean:
	rm $(TEST_EXE)
