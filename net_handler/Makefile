
CFLAGS=$(shell pkg-config --cflags libprotobuf-c)
LDFLAGS=$(shell pkg-config --libs libprotobuf-c)
GEN_FOLDER=pbc_gen
TEST_FOLDER=protobuf_tests

all: gen_proto log runmode gpstate devdata client server

gen_proto: protos/*.proto
	cd protos && protoc-c --c_out ../$(GEN_FOLDER) *.proto

clean:
	rm -f $(GEN_FOLDER)/*
	rm -f $(shell find protobuf_tests -type f ! -name "*.*")

log: log_in log_out
runmode: runmode_in runmode_out
gpstate: gpstate_in gpstate_out
devdata: devdata_in devdata_out

# $@ is variable to the recipe name

log_in: $(TEST_FOLDER)/log_in.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

log_out: $(TEST_FOLDER)/log_out.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

runmode_in: $(TEST_FOLDER)/runmode_in.c 
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

runmode_out: $(TEST_FOLDER)/runmode_out.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

devdata_in: $(TEST_FOLDER)/devdata_in.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

devdata_out: $(TEST_FOLDER)/devdata_out.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

gpstate_in: $(TEST_FOLDER)/gpstate_in.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

gpstate_out: $(TEST_FOLDER)/gpstate_out.c
	gcc $(GEN_FOLDER)/*.c $(TEST_FOLDER)/$@.c -o $(TEST_FOLDER)/$@ $(LDFLAGS)

client: networking_sanity/tcp_sanity_client.c pbc_gen/text.pb-c.c
	gcc $^ -o networking_sanity/$@ -lprotobuf-c

server: networking_sanity/tcp_sanity_server.c pbc_gen/text.pb-c.c
	gcc $^ -o networking_sanity/$@ -lprotobuf-c
