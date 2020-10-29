
LIBS=-pthread -lrt -Wall

FILES=shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c

all: test1 test2 shm_start shm_stop

test1: shm_wrapper_test1.c $(FILES)
	gcc $^ -o $@ $(LIBS)

test2: shm_wrapper_test2.c $(FILES)
	gcc $^ -o $@ $(LIBS)

shm_start: shm_start.c $(FILES)
	gcc $^ -o $@ $(LIBS)

shm_stop: shm_stop.c $(FILES)
	gcc $^ -o $@ $(LIBS)

clean:
	rm test1 test2 shm_start shm_stop
