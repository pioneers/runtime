# Starting point for building stuff in Runtime from the root directory

all:
	cd dev_handler && make
	cd network_switch && make
	cd shm_wrapper && make
	cd net_handler && make
	cd executor && make

clean:
	# executor needs to take care of its Cython build products
	cd executor && make clean
	rm -rf bin
	rm -rf build
