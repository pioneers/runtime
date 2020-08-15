# Change this to point to your specific Python version. Need >= 3.6
py=python3.7

CFLAGS=$(shell $(py)-config --cflags)
LDFLAGS=$(shell $(py)-config --ldflags)
PY_LIB=-l$(py)m
LIBS=-pthread -lrt -export-dynamic -fPIC -lprotobuf-c
#-rdynamic or -export-dynamic to make it dynamically linked


all: executor studentapi.c

executor: executor.c ../shm_wrapper/shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c ../net_handler/net_util.c ../net_handler/pbc_gen/text.pb-c.c
	gcc $(CFLAGS) $^ -o $@ $(LIBS) $(PY_LIB)

studentapi.c: studentapi.pyx runtime.pxd
	$(py) setup.py build_ext -i

test_api: studentapi test_studentapi.py
	- $(py) test_studentapi.py

clean:
	rm -f studentapi.c
	rm -f executor
	rm -f *.so
