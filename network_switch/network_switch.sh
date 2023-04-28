#!/bin/bash

# script takes two arguments, the first being the name of the network to connect to,
# the second being the password to that network

# script returns 1 if successfully connected, 0 if unsuccessful

# forget all networks by removing all network configuration files in nmcli folders
sudo rm /etc/NetworkManager/system-connections/*

# initiate rescan of networks; this command only starts the scan, it doesn't wait for scan to complete
sudo nmcli dev wifi rescan

# wait 3 seconds, look for the network name in the output of nmcli d wifi list
# repeat a maximum of 3 times (arbitrarily chosen nmumbers)
pass=0
for i in {1..3}; do
    # if passed, break early
    if [[ $pass == 1 ]]; then
        break;
    fi

    sleep 3
    printf "Try number $i ...\n"

    while read line; do
        if [[ $line == *"$1"* ]]; then
            pass=1
            break
        fi
    done <<< "$(nmcli d wifi list)"
done

printf "Going to connect!\n"

# connect to the network; again do not proceed before action is complete
sudo nmcli d wifi connect $1 password $2

# we use the command nmcli -t -f CONNECTION,STATE device to get the state of NetworkManager
# we look for a line that is exactly <network_name>:connected
# for example, if successfully connect to pioneers, the output will contain:
# pioneers:connected
pass=0
while read line; do
    if [[ $line == "$1:connected" ]]; then
        pass=1
        break
    fi
done <<< "$(nmcli -t -f CONNECTION,STATE device)"

# write "1" or "0" to exit_status.txt to give the C program the result of this script
if [[ $pass == 1 ]]; then
    printf "1" > exit_status.txt
else
    printf "0" > exit_status.txt
fi

exit $pass
