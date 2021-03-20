#!/bin/bash

# This script/document automates *most* of the process for copying the SD card of a Raspberry Pi with a 
# working Runtime on it to other SD cards for more Raspberry Pi's.

# First step (only need to do this once): copy working SD card off of Raspberry Pi onto machine
# - On a mac, follow these instructions: https://www.raspberrypi.org/documentation/installation/installing-images/mac.md
# - IMPORTANT! Since you're copying FROM the Raspberry Pi TO your computer, for the dd command, switch the if=/ and of=/, i.e.
# --> sudo dd bs=1m if=/dev/rdiskN of=path_of_your_image.img; sync . Name the image whatever you want

# Repeat these steps for each Raspberry Pi to be provisioned:
sudo dd bs=1m if=path_of_your_image.img of=/dev/rdiskN; sync  # where path_of_your_image is from above and N is the number of the SD card in diskutil list

# Insert SD card into Raspberry Pi, plug in and ssh in. It will have the same hostname as the original Raspberry Pi

# Change date
# --> sudo date -s "20 MAR 2021 14:23:50" <-- (for example)
# Change timezone
# --> sudo raspi-config; select Localisation Options
# Ensure connection to WiFi (e.g. do ping google.com)

sudo rm -rf runtime
git clone https://github.com/pioneers/runtime.git
cd runtime && ./runtime build

# enable systemd services by doing the commands in runtime/systemd README
# (you can't put these commands in scripts)
# --> cd runtime/systemd
# --> sudo systemctl disable *.service
# --> sudo ln -s $HOME/runtime/systemd/*.service /etc/systemd/system/
# --> sudo systemctl enable *.service

# if using ngrok, change the authtoken in runtime/systemd/ngrok.yml

# TODO: Put something here to change the hostname to the argument to the script
# --> sudo hostname <new_hostname>

# reboot machine: 
# --> sudo reboot now