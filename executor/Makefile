cc=gcc
# Change this to point to your specific Python version. Need >= 3.6
py=python3.6

CFLAGS=$(shell $(py)-config --cflags)
LDFLAGS=$(shell $(py)-config --ldflags)
PY_LIB=-l$(py)m

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt
endif
ifeq ($(UNAME), Darwin)
LIBS=
endif

func: run_python.c
	$(cc) run_python.c -o func $(PY_LIB)

simple: run_simple_python.c
	$(cc) run_simple_python.c -o simple $(PY_LIB)

executor: executor.c ../shm_wrapper/shm_wrapper.c ../shm_wrapper_aux/shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h ../runtime_util/runtime_util.c
	$(cc) executor.c $(CFLAGS) $(LDFLAGS) ../shm_wrapper/shm_wrapper.c ../shm_wrapper_aux/shm_wrapper_aux.c ../logger/logger.c ../runtime_util/runtime_util.c -o executor $(LIBS)

studentapi: studentapi.pyx studentapi.pxd
	$(py) setup.py build_ext -i

test_api: studentapi
	$(py) test_studentapi.py

clean:
	rm studentapi.c
	rm simple
	rm func
	rm executor
	# rm studentapi.cpython-36m-x86_64-linux-gnu.so