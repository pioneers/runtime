#!/bin/bash

# move into folder with all the network names the potato connected to previously 
cd /etc/NetworkManager/system-connections
sudo rm * # remove all network names
sudo nmcli d wifi connect $1 password $2 # connect to wifi determined by network switch 

# keep trying to connect until wifi is connected, check by looping through output of nmcli d 