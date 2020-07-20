#!/bin/bash

# (I apologize to anyone who needs to edit this in the future.... bash is really stupid sometimes and cryptic)
# Some of the sequences of commands could be shortened or abbreviated but I tried to make it as easy to undersatnd as possible
# 
# Useful links for understanding this file:
# arduino-cli:
#	https://arduino.github.io/arduino-cli/getting-started/
# awk:
# 	https://opensource.com/article/19/11/how-regular-expressions-awk
#	https://linuxhint.com/20_awk_examples/
#	https://www.howtogeek.com/562941/how-to-use-the-awk-command-on-linux/
# bash:
#	https://linuxconfig.org/bash-scripting-tutorial
#	https://tldp.org/LDP/abs/html/string-manipulation.html
#	https://unix.stackexchange.com/questions/306111/what-is-the-difference-between-the-bash-operators-vs-vs-vs
# getopts:
#	https://stackoverflow.com/questions/16483119/an-example-of-how-to-use-getopts-in-bash
#	https://www.mkssoftware.com/docs/man1/getopts.1.asp
# od:
#	https://serverfault.com/questions/283294/how-to-read-in-n-random-characters-from-dev-urandom
# sed:
#	https://www.grymoire.com/Unix/Sed.html

####################################### GLOBAL VARIABLES ####################################

SENSOR_NAME="empty"       # DummyDevice, YogiBear, etc.
SENSOR_TYPES="empty"      # list of all sensor names
DEVICE_PORT="empty"       # port that board is connected on
DEVICE_FQBN="empty"       # fully qualified board name, ex. arduino:avr:micro
DEVICE_CORE="empty"       # core for connected board, ex. arduino:avr
LIB_INSTALL_DIR="empty"   # installation directory for libraries
uid="empty"               # UID of device (lowercase because capital UID is taken by system)
CLEAN=0                   # whether or not the clean option was specified

####################################### UTILITY FUNCTIONS ###################################

# prints the usage of this flash script
function usage {
	printf "Usage: ./flash.sh [-cth] [-s UID] sensor_type\n"
	printf "\t-s: short for \"Set\"; manually set the UID for the device (input taken in hex, do not prefix \"0x\")\n"
	printf "\t-t: short for \"Test\"; set the UID for the device to \"0x123456789ABCDEF\"\n"
	printf "\t-c: short for \"Clean\"; removes the \"lowcar/Device\" folder and all symbolic links\n"
	printf "\t-h: display this help message\n"
	printf "Valid values for \"sensor type\" are listed below:\n\n"
	
	for type in $SENSOR_TYPES; do
		printf "\t$type\n"
	done
	printf "\n"
	exit 1
}

# checks whether a sensor is valid; if so, put it in SENSOR_NAME
function check_sensor {
	# check if user specified a sensor type
	if [[ $1 == "" ]]; then
		printf "\nERROR: Must specify a sensor type\n\n"
		usage
		exit 1
	fi
	
	# check if sensor is valid
	for sensor in ${SENSOR_TYPES}; do
		if [[ $1 == $sensor ]]; then
			SENSOR_NAME=$sensor
			return 0
		fi
	done
	
	printf "\nERROR: Invalid sensor type: $1\n\n"
	usage
	exit 1
}

# parse command-line options
function parse_opts {
	# fill in SENSOR_TYPES global variable
	SENSOR_TYPES=$(ls lib/lowcar | awk '{ 
		for (i=1; i <= NF; i++) {
			tmp=match($i, /^Device$/)
			if (!tmp) {
				print $i
			}
		}
	}')
	
	# process the command-line arguments
	while getopts "s:cth" opt; do
		case $opt in
			s)
				# check to make sure that s flag was specified with hexadecimal digits
				if [[ $OPTARG =~ [:xdigit:] ]]; then
					printf "\nERROR: Specified invalid UID for -s flag: \"$uid\"\n\n"
					usage
				fi
				uid="0x$OPTARG"
				;;
			t)
				uid="0x123456789ABCDEF"
				;;
			c)
				# check to make sure user didn't specify more arguments after -c flag
				if [[ $OPTIND == $# ]]; then
					printf "\nERROR: -c flag does not take any arguments\n\n"
					usage
				fi
				CLEAN=1
				return 0
				;;
			*)
				usage
				;;
		esac
	done
	shift $(( $OPTIND - 1 ))  # shift the command-line arguments so that $1 refers to the sensor type
	
	check_sensor $1 # check that we have a valid sensor
	
	return 0
}

# obtains the port, fqbn, and core of the attached arduino
function get_board_info {
	arduino-cli board list > temp1.txt
	local line_num=0
	
	# read lines from temp1.txt and searches for the attached board
	while read line; do
		line_num=$(( line_num + 1 ))
		
		# if first line, or line contains "Unknown" or is blank, then skip
		if [[ $line_num == 1 || $line == *"Unknown"* || $line == "" ]]; then
			continue
		fi
		
		# we found the board! now fill in the global variables
		DEVICE_PORT=$(echo $line | awk '{ print $1 }')
		DEVICE_FQBN=$(echo $line | awk '{ 
				for (i=1; i<=NF; i++) {
					tmp=match($i, /arduino:(avr|samd):.*/) 
					if (tmp) {
						print $i
					} 
				} 
			}')
		DEVICE_CORE=$(echo $line | awk '{ 
				for (i=1; i<=NF; i++) {
					tmp=match($i, /arduino:[^:]*$/) 
					if (tmp) {
						print $i
					} 
				} 
			}')
		
		rm temp1.txt
		
		printf "\nFound board! \n\t Port = $DEVICE_PORT \n\t FQBN = $DEVICE_FQBN \n\t Core = $DEVICE_CORE\n\n"
		
		return 0
		
	done < "temp1.txt"
	
	# if we go through the entire output of arduino-cli board list, then no suitable board attached
	rm temp1.txt
	printf "\nERROR: no board attached\n\n"
	exit 1
}

# get the location that we need to symlink our code from so that arduino-cli can find them
# function get_lib_install_dir {
# 	# get the configuration into temp1.txt
# 	arduino-cli config dump > temp1.txt
#
# 	# read in temp1.txt line by line until we find the "user: LIB_INSTALL_DIR" line
# 	while read line; do
# 		if [[ $line == *"user"* ]]; then
# 			echo $line > temp2.txt
# 			LIB_INSTALL_DIR=$(awk '{ print $2 }' temp2.txt)
# 			LIB_INSTALL_DIR="$LIB_INSTALL_DIR/libraries" # append /libraries to it
# 			rm temp1.txt temp2.txt
# 			printf "\nFound Library Installation Directory: $LIB_INSTALL_DIR\n"
# 			return 0
# 		fi
# 	done < "temp1.txt"
#
# 	# if we get here, we didn't find the library installation directory
# 	rm temp1.txt
# 	printf "\nERROR: could not find Library Installation Directory\n\n"
# 	exit 1
# }

function get_lib_install_dir {
	# install a library and record the logs of the installation into temp1.txt
	arduino-cli lib install --log-file temp1.txt MFRC522 > /dev/null
	arduino-cli lib uninstall MFRC522 > /dev/null
	
	# read in temp1.txt line by line until we find the "location=user"
	# the word before that is going to be "dir=LIB_INSTALL_DIR"
	local prev_word="empty"
	while read line; do
		for word in $line; do
			if [[ $word == "location=user" ]]; then
				LIB_INSTALL_DIR=$(echo $prev_word | awk -F= '{ print $2 }')
				printf "\nFound Library Installation Directory: $LIB_INSTALL_DIR\n"
				rm temp1.txt
				return 0
			fi
			prev_word=$word
		done
	done < "temp1.txt"

	# if we get here, we didn't find the library installation directory
	rm temp1.txt
	printf "\nERROR: could not find Library Installation Directory\n\n"
	exit 1
}

# create the symbolic links from the library installation directory to our code
function symlink_libs {
	local cwd=$(pwd)
	
	# first get names of folders to symlink in libs_to_install and lowcar_libs_to_install
	local libs_to_install=$(ls lib)
	local lowcar_libs_to_install=$(ls lib/lowcar)
	
	printf "\nCreating symbolic links...\n\n"
	
	# process each element in libs_to_install
	for lib in $libs_to_install; do
		# skip "lowcar"
		if [[ $lib == "lowcar" ]]; then
			continue
		fi
		
		# now check if it already exists in LIB_INSTALL_DIR
		if [[ -L "$LIB_INSTALL_DIR/$lib" ]]; then
			continue
		fi
		
		printf "Creating symbolic link for $lib\n"
		ln -s "$cwd/lib/$lib" "$LIB_INSTALL_DIR/$lib"
	done
	
	# process each element in lowcar_libs_to_install
	for lib in $lowcar_libs_to_install; do
		# now check if it already exists in LIB_INSTALL_DIR
		if [[ -L "$LIB_INSTALL_DIR/$lib" ]]; then
			continue
		fi
		
		printf "Creating symbolic link for $lib\n"
		ln -s "$cwd/lib/lowcar/$lib" "$LIB_INSTALL_DIR/$lib"
	done
	
	printf "\nFinished creating symbolic links to libraries!\n"
}

# sets the UID of this device and generates the finished Arduino sketch code
function make_device_ino {
	# if UID hasn't been set yet, generate a random one
	if [[ $uid == "empty" ]]; then
		printf "\nGenerating random UID\n"
	
		# get the random 64-bit UID
		if [[ -e "/dev/urandom" ]]; then
			uid=$(od -tu8 -An -N8 /dev/urandom) # get 8 bytes of random data from /dev/urandom
		else
			uid=$(od -tu8 -An -N8 /dev/random) # get 8 bytes of random data from /dev/random
		fi
		
		# this trims the whitespace surrounding rand so that generated file looks nicer
		uid="$(echo $uid | tr -d '[:space:]')"
	fi
	
	echo "#include <$SENSOR_NAME.h>" > temp1.txt
	
	printf "UID for this device is $uid\n"
	printf "Generating Device/Device.ino\n"
	
	# generates new Device_template file with UID_INSERTED and DEVICE replaced
	sed -e "s/UID_INSERTED/$uid/" -e "s/DEVICE/$SENSOR_NAME/" Device_template.cpp > temp2.txt
	
	# if Device folder doesn't exist, make it
	if [[ ! -d "Device" ]]; then
		mkdir Device
	fi
	
	# concatenate temp1.txt and temp2.txt into Device/Device.ino
	cat temp1.txt temp2.txt > Device/Device.ino
	
	rm temp1.txt temp2.txt
	
	printf "Ready to attempt compiling!\n"
}

# removes the lowcar/Device folder and all symbolic links
function clean {
	# get the location of library install
	get_lib_install_dir
	
	# get the names of libraries to remove
	local libs_to_clean="$(ls lib) $(ls lib/lowcar)"
	
	# remove lowcar/Device if it exists
	if [[ -d "Device" ]]; then
		printf "Removing lowcar/Device\n"
		rm -r Device
	fi
	
	# process each element in libs_to_clean
	for lib in $libs_to_clean; do
		# skip "lowcar"
		if [[ $lib == "lowcar" ]]; then
			continue
		fi
		
		# now check if it already exists in LIB_INSTALL_DIR
		if [[ -L "$LIB_INSTALL_DIR/$lib" ]]; then
			printf "Removing symbolic link for $lib\n"
			rm "$LIB_INSTALL_DIR/$lib"
		fi
		
	done
	
	printf "Finished cleaning!\n\n"
	
	exit 1
}

####################################### BEGIN SCRIPT ###################################

# parse command-line options
parse_opts $@

# check if CLEAN flag was specified
if [[ $CLEAN == 1 ]]; then
	clean
fi

printf "\nAttempting to flash a $SENSOR_NAME!\n\n"

# create the init file (see arduino-cli documentation)
arduino-cli config init

# update the core index (see arduino-cli documentation)
arduino-cli core update-index

# get the device port, board, fqbn, and core
get_board_info

# install this device's core
arduino-cli core install $DEVICE_CORE

# symlink libraries
get_lib_install_dir
symlink_libs

# insert uid and requested device into Device_template, copy into Device/Device.ino
make_device_ino

# compile the code
printf "\nCompiling code...\n\n"
if !(arduino-cli compile --fqbn $DEVICE_FQBN Device/Device.ino); then
	printf "\nERROR: code did not compile correctly\n\n"
	exit 1
fi
printf "\nFinished compiling code!\n"

# upload the code to board
printf "\nUploading code to board...\n\n"
if !(arduino-cli upload -p $DEVICE_PORT --fqbn $DEVICE_FQBN Device); then
	printf "\nERROR: code did not upload correctly\n\n"
	exit 1
fi
printf "Finished uploading code!\n"

# done!
printf "\nSuccessfully flashed a $SENSOR_NAME!\n\n"

exit 0
