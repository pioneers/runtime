cc=gcc
CFLAGS=$(python3.6-config --cflags)
LDFLAGS=$(python3.6-config --ldflags)
PYTHON=-lpython3.6m
LIBS=-pthread -lrt

func: run_python.c
	cc run_python.c -o func $(PYTHON)

simple: run_simple_python.c
	cc run_simple_python.c -o simple $(PYTHON)

executor: executor.c ../shm_wrapper/shm_wrapper.c ../shm_wrapper_aux/shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h ../runtime_util/runtime_util.c
	cc executor.c ../shm_wrapper/shm_wrapper.c ../shm_wrapper_aux/shm_wrapper_aux.c ../logger/logger.c ../runtime_util/runtime_util.c -o executor $(PYTHON) $(LIBS)

studentapi: studentapi.pyx studentapi.pxd
	python3.6 setup.py build_ext -i

test_api: studentapi
	python3.6 test_studentapi.py

clean:
	rm studentapi.c
	rm studentapi.cpython-36m-x86_64-linux-gnu.so
	rm simple
	rm func
	rm executor