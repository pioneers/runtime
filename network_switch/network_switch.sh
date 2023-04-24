#!/bin/bash

cd /etc/NetworkManager/system-connections
sudo rm *
sudo nmcli d wifi connect $1 password $2 

#keep trying to connect until wifi is connected, check by looping through output of nmcli d 