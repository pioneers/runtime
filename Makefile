SHELL := /bin/bash


build:
	pushd executor && make && popd
	pushd net_handler && make && popd
	pushd shm_wrapper && make && popd
	pushd dev_handler && make && popd

run: build
	./run.sh

clean:
	pushd executor && make clean && popd
	pushd net_handler && make clean && popd
	pushd shm_wrapper && make clean && popd
	pushd dev_handler && make clena && popd

test:
	./test.sh
