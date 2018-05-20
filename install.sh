#!/usr/bin/env bash

sudo apt-get -y update
sudo apt-get install -y git ntp g++ potrace

sudo service ntp stop
ntpdate -s ntp.ubuntu.com
sudo service ntp start

./install_blender.sh
./install_vision.sh

