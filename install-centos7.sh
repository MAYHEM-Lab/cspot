#!/bin/bash


yum -y install centos-release-scl && yum -y install devtoolset-9
yum -y install centos-release-scl && sudo yum -y install devtoolset-9
yum -y install glibc-static
cd ..
wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
unzip ninja-linux.zip 
mv ninja /usr/bin/
wget https://github.com/Kitware/CMake/releases/download/v3.19.1/cmake-3.19.1-Linux-x86_64.sh
chmod +x cmake-3.19.1-Linux-x86_64.sh 
./cmake-3.19.1-Linux-x86_64.sh --skip-license --prefix=/usr
#git clone --branch release-2.0 https://github.com/Mayhem-lab/cspot
cd cspot/
git submodule update --init --recursive
mkdir build
cd build/
echo "#!/bin/bash" > helper.sh
echo "cmake -G Ninja .." >> helper.sh
echo "help" >> helper.sh
chmod 755 helper.sh
scl enable devtoolset-9 ./helper.sh

