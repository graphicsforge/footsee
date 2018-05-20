#!/usr/bin/env bash

set -e
set -x

sudo add-apt-repository -y ppa:thomas-schiex/blender
sudo apt-get -y update
# sudo apt-get -y libglu1-mesa libxi6
sudo apt-get -y install blender=2.76-b-1446548197-0thomas~trusty1
