
CFLAGS=$(shell pkg-config --cflags libprotobuf-c)
LDFLAGS=$(shell pkg-config --libs libprotobuf-c)
GEN_FOLDER=pbc_gen
TEST_FOLDER=protobuf_tests

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
LIBS=-pthread -lrt
#-rdynamic or -export-dynamic to make it dynamically linked
endif
ifeq ($(UNAME), Darwin)
LIBS=
endif

all: gen_proto log runmode gpstate devdata client server

gen_proto: $(wildcard protos/*.proto)
	cd protos && protoc-c --c_out ../$(GEN_FOLDER) *.proto

clean:
	rm -f $(GEN_FOLDER)/*
	rm -f $(shell find protobuf_tests -type f ! -name "*.*")

log: log_in log_out
runmode: runmode_in runmode_out
gpstate: gpstate_in gpstate_out
devdata: devdata_in devdata_out

# $@ is variable to the recipe name
%: $(TEST_FOLDER)/%.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

client: networking_sanity/tcp_sanity_client.c pbc_gen/text.pb-c.c
	gcc $^ -o networking_sanity/$@ $(LDFLAGS)

server: networking_sanity/tcp_sanity_server.c pbc_gen/text.pb-c.c
	gcc $^ -o networking_sanity/$@ $(LDFLAGS)

udp_suite: udp_suite.c $(wildcard $(GEN_FOLDER)/*.c) ../runtime_util/runtime_util.c ../logger/logger.c ../shm_wrapper/shm_wrapper.c ../shm_wrapper_aux/shm_wrapper_aux.c
	gcc $^ -o udp_suite $(LIBS) $(LDFLAGS)
