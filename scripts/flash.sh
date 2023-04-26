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

DEVICE_NAME="empty"       # DummyDevice, YogiBear, etc.
DEVICE_PORT="empty"       # port that board is connected on
DEVICE_FQBN="empty"       # fully qualified board name, ex. arduino:avr:micro
DEVICE_CORE="empty"       # core for connected board, ex. arduino:avr
LIB_INSTALL_DIR="empty"   # installation directory for libraries
uid="empty"               # UID of device (lowercase because capital UID is taken by system)
CLEAN=0                   # whether or not the clean option was specified

####################################### UTILITY FUNCTIONS ###################################

# prints the usage of this flash script
function usage {
    printf "Usage: flash [-cht] [-s UID] [-p port] device_type\n"
    printf "\t-c: short for \"Clean\"; removes the \"Device\" folder and all symbolic links\n"
    printf "\t-h: short for \"Help\": displays this help message\n"
    printf "\t-p: short for \"Port\": manually specify the port file path of the Arduino to flash\n"
    printf "\t-s: short for \"Set\"; manually set the UID for the device (input taken in hex, do not prefix \"0x\")\n"
    printf "\t-t: short for \"Test\"; set the UID for the device to \"0x123456789ABCDEF\"\n\n"
    printf "Examples:\n"
    printf "\tTo flash a DummyDevice: flash DummyDevice\n"
    printf "\tTo flash a DummyDevice specifying the UID \"ABCDEF\": flash -s ABCDEF DummyDevice\n"
    printf "\tTo flash a DummyDevice with the test UID \"123456789ABCDEF\": flash -t DummyDevice\n"
    printf "\tTo flash a DummyDevice that is at the port \"/dev/ttyACM1\": flash -p /dev/ttyACM1 DummyDevice\n"
    printf "\tTo clean up directories without flashing anything: flash -c\n\n"
    printf "Valid values for \"device type\" are listed below:\n\n"
    
    for type in $DEVICE_TYPES; do
        printf "\t$type\n"
    done
    printf "\n"
    
    printf "Valid values for \"port\" are listed below:\n\n"
    num_ports=0 # counts the number of valid ports for the -p flag
    line_num=0
    while read line; do
        line_num=$(( line_num + 1 ))
        
        # if first line, or line contains "Unknown" or is blank, then skip
        if [[ $line_num == 1 || $line == *"Unknown"* || $line == "" ]]; then
            continue
        fi
        
        # arduino-cli has reported a flashable Arduino; increment num_ports and print the port path
        num_ports=$(( num_ports + 1 ))
        device_port=$(echo $line | awk '{ print $1 }') # save the port name in this line temporarily
        printf "\t$device_port\n"
    done <<< "$(arduino-cli board list)"
    
    # Print a message if there are no valid ports
    if [[ $num_ports == 0 ]]; then
        printf "\tFlash script could not find any flashable Arduinos attached to this computer!\n"
    fi
    printf "\n"
    
    exit 1
}

# checks whether a device is valid; if so, put it in DEVICE_NAME
function check_device {
    # check if user specified a device type
    if [[ $1 == "" ]]; then
        printf "\nERROR: Must specify a device type\n\n"
        usage
        exit 1
    fi
    
    # check if device is valid
    for device in ${DEVICE_TYPES}; do
        if [[ $1 == $device ]]; then
            DEVICE_NAME=$device
            return 0
        fi
    done
    
    printf "\nERROR: Invalid device type: $1\n\n"
    usage
    exit 1
}

# parse command-line options
function parse_opts {
    # process the command-line arguments
    while getopts "p:s:cth" opt; do
        case $opt in
            p)
                # check to make sure that the p flag was specified with valid path
                if [[ -e $OPTARG ]]; then
                    DEVICE_PORT=$OPTARG
                    printf "\nSpecified device port path $OPTARG exists\n\n"
                else
                    printf "\nERROR: Specified device port path $OPTARG does not exist\n\n"
                    usage
                fi
                ;;
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
                if (( $OPTIND <= $# )); then
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
    shift $(( $OPTIND - 1 ))  # shift the command-line arguments so that $1 refers to the device type
    
    check_device $1 # check that we have a valid device
    
    return 0
}

# obtains the port, fqbn, and core of the attached arduino
function get_board_info {
    local line_num=0
    
    # read lines from board list and searches for the attached board
    while read line; do
        line_num=$(( line_num + 1 ))
        
        # if first line, or line contains "Unknown" or is blank, then skip
        if [[ $line_num == 1 || $line == *"Unknown"* || $line == "" ]]; then
            continue
        fi
        
        # we found the board! now fill in the global variables
        device_port_temp=$(echo $line | awk '{ print $1 }') # save the port name in this line temporarily
        
        # if -p flag was specified and the specified path is not in this line of output, skip it
        if [[ ($DEVICE_PORT != "empty") && ($DEVICE_PORT != $device_port_temp) ]]; then
            continue
        fi
        
        DEVICE_PORT=$device_port_temp
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
        
        printf "\nFound board! \n\t Port = $DEVICE_PORT \n\t FQBN = $DEVICE_FQBN \n\t Core = $DEVICE_CORE\n\n"
        
        return 0
        
    done <<< "$(arduino-cli board list)"
    
    # if we go through the entire output of arduino-cli board list, then no suitable board attached
    printf "\nERROR: no board attached\n\n"
    exit 1
}

# get the location that we need to symlink our code from so that arduino-cli can find them
function get_lib_install_dir {	
    
    # read in config line by line until we find the "user: LIB_INSTALL_DIR" line	
    while read line; do	
        if [[ $line == "user"* ]]; then	
            LIB_INSTALL_DIR=$(echo $line | awk '{ print $2 }')	
            LIB_INSTALL_DIR="$LIB_INSTALL_DIR/libraries" # append /libraries to it	
            mkdir -p $LIB_INSTALL_DIR # Make library directory if it doesn't exist
            printf "\nFound Library Installation Directory: $LIB_INSTALL_DIR\n"	
            return 1
        fi	
    done <<< "$(arduino-cli config dump)"
    
    # if we get here, we didn't find the library installation directory	
    printf "\nERROR: could not find Library Installation Directory\n\n"	
    exit 1	
}

# create the symbolic links from the library installation directory to our code
# "$PWD" is set by the system and contains the output of the "pwd" command in the terminal
function symlink_libs {
    
    # first get names of folders to symlink in libs_to_install and devices_libs_to_install
    local libs_to_install=$(ls lib)
    local devices_to_install=$(ls devices)
    printf "\nCreating symbolic links...\n\n"
    
    # process each element in libs_to_install
    for lib in $libs_to_install; do	
        # now check if it already exists in LIB_INSTALL_DIR
        if [[ -L "$LIB_INSTALL_DIR/$lib" ]]; then
            continue
        fi
        
        printf "Creating symbolic link for $lib\n"
        ln -s "$PWD/lib/$lib" "$LIB_INSTALL_DIR/$lib"
    done
    
    # process each element in devices_to_install
    for lib in $devices_to_install; do
        # now check if it already exists in LIB_INSTALL_DIR
        if [[ -L "$LIB_INSTALL_DIR/$lib" ]]; then
            continue
        fi
        
        printf "Creating symbolic link for $lib\n"
        ln -s "$PWD/devices/$lib" "$LIB_INSTALL_DIR/$lib"
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
    
    printf "UID for this device is $uid\n"
    printf "Generating Device/Device.ino\n"
    
    # if Device folder doesn't exist, make it
    mkdir -p Device
    
    # create Device/Device.ino with proper header
    echo "#include <$DEVICE_NAME.h>" > Device/Device.ino
    # generates new Device_template file with UID_INSERTED and DEVICE replaced and appends to existing Device.ino
    sed -e "s/UID_INSERTED/$uid/" -e "s/DEVICE/$DEVICE_NAME/" Device_template.cpp >> Device/Device.ino
        
    printf "Ready to attempt compiling!\n"
}

# removes the Device folder and all symbolic links
function clean {
    # get the location of library install
    get_lib_install_dir
    
    # get the names of libraries to remove
    local libs_to_clean="$(ls lib) $(ls devices)"
    
    # remove Device if it exists
    if [[ -d "Device" ]]; then
        printf "Removing Device\n"
        rm -r Device
    fi
    
    # process each element in libs_to_clean
    for lib in $libs_to_clean; do
        # now check if it already exists in LIB_INSTALL_DIR
        if [[ -L "$LIB_INSTALL_DIR/$lib" ]]; then
            printf "Removing symbolic link for $lib\n"
            rm "$LIB_INSTALL_DIR/$lib"
        fi
        
    done
    
    printf "Finished cleaning!\n\n"	
}

####################################### BEGIN SCRIPT ###################################

# This script should only be called using `runtime flash` which will have its CWD be runtime/. We need to be in the lowcar/ folder.
cd lowcar

# Get all valid device types
DEVICE_TYPES=$(ls devices | awk '{ 
        for (i=1; i <= NF; i++) {
            tmp=match($i, /^Device$/)
            if (!tmp) {
                print $i
            }
        }
    }'
)

# check if arduino-cli is installed
if [[ $(which arduino-cli) == "" ]]; then
    printf "\nERROR: Could not find arduino-cli. Please install arduino-cli\n\n"
    usage
    exit 1
fi

# parse command-line options
parse_opts $@

# Clean symlinked libraries
clean

# if CLEAN flag was specified, we are done
if [[ $CLEAN == 1 ]]; then
    exit 1
fi

printf "\nAttempting to flash a $DEVICE_NAME!\n\n"

# create the init file (see arduino-cli documentation)
arduino-cli config init

# update the core index (see arduino-cli documentation)
arduino-cli core update-index --override

# symlink libraries
symlink_libs

# insert uid and requested device into Device_template, copy into Device/Device.ino
make_device_ino

# get the device port, board, fqbn, and core
get_board_info

# install this device's core
arduino-cli core install $DEVICE_CORE

# compile the code
printf "\nCompiling code...\n\n"
if !(arduino-cli compile --fqbn $DEVICE_FQBN Device); then
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
printf "\nSuccessfully flashed a $DEVICE_NAME!\n\n"

exit 0
