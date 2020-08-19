  
UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt -Wall
endif
ifeq ($(UNAME), Darwin)
LIBS=-Wall
endif

FILES=shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c

all: test1 test2 static_dev static_aux shm_start shm_stop

test1: shm_wrapper_test1.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
test2: shm_wrapper_test2.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
shm_start: shm_start.c $(FILES)
	gcc $^ -o $@ $(LIBS)

shm_stop: shm_stop.c $(FILES)
	gcc $^ -o $@ $(LIBS)

static_dev: static_dev.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
static_aux: static_aux.c $(FILES)
	gcc $^ -o $@ $(LIBS)

clean:
	rm test1 test2 shm_start shm_stop static_dev static_aux