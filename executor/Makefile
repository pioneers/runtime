cc=gcc
# Change this to point to your specific Python version. Need >= 3.6
py=python3.7

CFLAGS=$(shell $(py)-config --cflags)
LDFLAGS=$(shell $(py)-config --ldflags)
PY_LIB=-l$(py)m

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -export-dynamic
#-rdynamic or -export-dynamic to make it dynamically linked
endif
ifeq ($(UNAME), Darwin)
LIBS=-export-dynamic
endif


all: executor studentapi

executor: executor.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c
	$(cc) $(CFLAGS) $^ -o $@ $(LIBS) $(PY_LIB)

studentapi: studentapi.pyx runtime.pxd code_parser.pyx
	$(py) setup.py build_ext -i

test_api: studentapi test_studentapi.py
	- $(py) test_studentapi.py

clean:
	rm -f studentapi.c
	rm -f executor
	rm -f *.so
