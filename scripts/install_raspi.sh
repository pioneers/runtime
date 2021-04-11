#!/bin/bash

# This script/document automates *most* of the process for installing Runtime on a blank Raspberry Pi
# that has only the non-GUI Raspbian image copied onto its SD card. To copy that image, do the following:
# - https://www.raspberrypi.org/software/operating-systems/ Download the "Raspberry Pi OS Lite"
# - Unzip the zip on your computer
# - On a Mac, follow these instructions: https://www.raspberrypi.org/documentation/installation/installing-images/mac.md
# - Before ejecting, go to:
# --> cd /Volumes/boot
# --> touch ssh
# --> cd ..
# - Then eject. Those steps enable ssh'ing into the rapsberry pi

# ssh into Raspberry Pi

# Change date
# --> sudo date -s "20 MAR 2021 14:23:50" <-- (for example)
# Change timezone
# --> sudo raspi-config; select Localisation Options
# Connect to WiFi
# --> sudo raspi-config; select Network Options

# update apt-get, install git, clone runtime onto Raspi
sudo apt-get update
sudo apt-get -y install git
git clone https://github.com/pioneers/runtime.git

# install Python devtools and Cython
sudo apt-get -y install python3-dev
sudo apt-get -y install python3-pip
python3 -m pip install Cython

# install protobuf
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.15.1/protobuf-cpp-3.15.1.tar.gz
tar -xzf protobuf-cpp-3.15.1.tar.gz
cd protobuf-3.15.1 && ./configure && make && sudo make install && sudo ldconfig

# install protobuf-c
wget https://github.com/protobuf-c/protobuf-c/releases/download/v1.3.3/protobuf-c-1.3.3.tar.gz
tar -xzf protobuf-c-1.3.3.tar.gz
cd protobuf-c-1.3.3 && ./configure && make && sudo make install && sudo ldconfig

# install other Runtime dependencies with apt-get
sudo apt-get -y install libncurses5-dev libncursesw5-dev
sudo apt-get -y install clang-format

# build runtime
cd runtime && ./runtime build

# enable systemd services by doing the commands in runtime/systemd README
# (you can't put these commands in scripts)
# --> cd runtime/systemd
# --> sudo systemctl disable *.service
# --> sudo ln -s $HOME/runtime/systemd/*.service /etc/systemd/system/
# --> sudo systemctl enable *.service

# reboot machine: 
# --> sudo reboot now