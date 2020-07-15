UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt
endif
ifeq ($(UNAME), Darwin)
LIBS=
endif

FILES=shm_wrapper.c ../logger/logger.c ../runtime_util/runtime_util.c

test1: shm_wrapper_test1.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
test2: shm_wrapper_test2.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
shm: shm.c $(FILES)
	gcc $^ -o $@ $(LIBS)

static_dev: static_dev.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
static_aux: static_aux.c $(FILES)
	gcc $^ -o $@ $(LIBS)

print: print_shm.c $(FILES)
	gcc $^ -o $@ $(LIBS)
