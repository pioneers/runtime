UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt
endif
ifeq ($(UNAME), Darwin)
LIBS=
endif

FILES=shm_wrapper_aux.c ../logger/logger.c ../logger/logger_config.h ../runtime_util/runtime_util.c

test1: shm_wrapper_aux_test1.c $(FILES)
	gcc $^ -o $@ $(LIBS)
	
test2: shm_wrapper_aux_test2.c $(FILES)
	gcc $^ -o $@ $(LIBS)

static: connect_static_aux.c $(FILES)
	gcc $^ -o $@ $(LIBS)

print: print_aux_shm.c $(FILES)
	gcc $^ -o $@ $(LIBS)
