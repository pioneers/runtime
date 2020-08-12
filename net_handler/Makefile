CFLAGS=$(shell pkg-config --cflags libprotobuf-c)
LDFLAGS=$(shell pkg-config --libs libprotobuf-c)
LIBS=-pthread -lrt -Wall

GEN_FOLDER=pbc_gen
TEST_FOLDER=protobuf_tests
GEN_FILES:=$(GEN_FOLDER)/*.c


all: gen_proto net_handler udp_client tcp_client

gen_proto: $(wildcard protos/*.proto)
	cd protos && protoc-c --c_out ../$(GEN_FOLDER) *.proto

clean:
	rm -f $(GEN_FOLDER)/*
	rm -f $(shell find protobuf_tests -type f ! -name "*.*")
	rm -f net_handler
	rm -f tcp_client
	rm -f udp_client

proto_tests: gen_proto log runmode gpstate devdata

log: log_in log_out
runmode: runmode_in runmode_out
gpstate: gpstate_in gpstate_out
devdata: devdata_in devdata_out

# $@ is variable to the recipe name
%: $(TEST_FOLDER)/%.c
	gcc $(GEN_FILES) $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

run_%: $(TEST_FOLDER)/%.c
	@echo %
	%_out | %_in

client: protobuf_tests/tcp_pb_client.c pbc_gen/text.pb-c.c
	gcc $^ -o protobuf_tests/$@ $(LDFLAGS)

server: protobuf_tests/tcp_pb_server.c pbc_gen/text.pb-c.c
	gcc $^ -o protobuf_tests/$@ $(LDFLAGS)

net_handler: net_util.c net_handler.c udp_conn.c tcp_conn.c ../logger/logger.c ../shm_wrapper/shm_wrapper.c ../runtime_util/runtime_util.c 
	gcc $^ $(GEN_FILES) -o $@ $(LIBS) $(LDFLAGS) 

udp_client: test/udp_client.c ../logger/logger.c ../runtime_util/runtime_util.c
	gcc $^ $(GEN_FILES) -o $@ $(LIBS) $(LDFLAGS)

tcp_client: test/tcp_client.c net_util.c ../logger/logger.c ../runtime_util/runtime_util.c
	gcc $^ $(GEN_FILES) -o $@ $(LIBS) $(LDFLAGS)
