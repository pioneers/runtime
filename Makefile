# Starting point for building stuff in Runtime from the root directory

all:
	cd dev_handler && make
	cd network_switch && make

clean:
	rm -rf bin
	rm -rf build
