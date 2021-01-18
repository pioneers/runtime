LIBS=-pthread -lrt -Wall

FILES=shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c

all: shm_start shm_stop

shm_start: shm_start.c $(FILES)
	gcc $^ -o $@ $(LIBS)

shm_stop: shm_stop.c $(FILES)
	gcc $^ -o $@ $(LIBS)

clean:
	rm -f shm_start shm_stop
