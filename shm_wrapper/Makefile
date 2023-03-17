LIBS=-pthread -lrt -Wall

FILES=shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c
BIN=../bin

all: shm_start shm_stop

shm_start: shm_start.c $(FILES)
	gcc $^ -o $(BIN)/$@ $(LIBS)

shm_stop: shm_stop.c $(FILES)
	gcc $^ -o $(BIN)/$@ $(LIBS)

clean:
	rm -f $(BIN)/shm_start $(BIN)/shm_stop
