
# install pot race
sudo apt-get install -y potrace

# install imagemagick
sudo apt-get install -y git imagemagick

# install blender
sudo apt-get install -y blender

# install opencv
sudo apt-get install -y git make cmake g++

git clone --depth 1 https://github.com/Itseez/opencv.git
mkdir -p ./opencv/build
cd opencv/build
cmake ../
make
sudo make install
sudo ldconfig
cd ../..

# make a place for our vision binaries to go
mkdir -p ./build

# build our vision binaries
make
