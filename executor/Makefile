# list of libraries that executor needs to compile
LIBS=-pthread -lrt -Wall -export-dynamic -fPIC

# list of source files that the target (executor) depends on, relative to this folder
SRCS = executor.c gamestate_filter.c ../logger/logger.c ../runtime_util/runtime_util.c ../shm_wrapper/shm_wrapper.c

# Python compilation definitions
PY_VER = python3.11
PY_LIB = -l$(PY_VER)
CFLAGS = $(shell $(PY_VER)-config --cflags)

# specify the target (executable we want to make)
TARGET = executor

.PHONY: all clean $(TARGET)

all: $(TARGET) studentapi.c

# include top-level Makefile for common variables and rules
include ../Makefile_defs

$(TARGET): $(BIN)/$(TARGET)

# rule to compile executor
$(BIN)/$(TARGET): $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $@ $(LIBS) $(PY_LIB)

# rule to compile studentapi.c
studentapi.c: studentapi.pyx studentapi.pxd setup.py
	$(PY_VER) setup.py build_ext -i

# rule to compile testapi
test_api: studentapi.c test_studentapi.py
	- $(PY_VER) test_studentapi.py

# remove build artifacts
clean:
	rm -f $(OBJS)
	rm -f $(patsubst %.o,%.d,$(OBJS))
	rm -rf studentapi.c
	rm -f *.so
	rm -rf build
	rm -f $(BIN)/$(TARGET)

-include $(patsubst %.o,%.d,$(OBJS))
