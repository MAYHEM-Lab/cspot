#!/bin/bash


# turn off TESTS build in deps/libzmq/CMakeLists.txt to avoid linker confusion
# set SIGSTSZ to 16K in cspot/src/tests/doctest.h


dnf -y install gcc g++ wget scl-utils unzip
dnf -y install glibc-static
dnf -y install wget
dnf -y remove podman runc
curl https://download.docker.com/linux/centos/docker-ce.repo -o /etc/yum.repos.d/docker-ce.repo
sed -i -e "s/enabled=1/enabled=0/g" /etc/yum.repos.d/docker-ce.repo
dnf --enablerepo=docker-ce-stable -y install docker-ce
systemctl start docker
#dnf --enablerepo=crb install ninja-build
cd ..
wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
unzip ninja-linux.zip 
mv ninja /usr/bin/
wget https://github.com/Kitware/CMake/releases/download/v3.19.1/cmake-3.19.1-Linux-x86_64.sh
chmod +x cmake-3.19.1-Linux-x86_64.sh 
./cmake-3.19.1-Linux-x86_64.sh --skip-license --prefix=/usr
yum -y localinstall https://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/c/czmq-3.0.2-3.el7.x86_64.rpm
cd cspot/
git submodule update --init --recursive
echo "set(HAVE_SELECT 1)" > deps/libzmq/CMakeLists-notests.txt
sed 's/build the tests" ON/build the tests" OFF/' deps/libzmq/CMakeLists.txt >> deps/libzmq/CMakeLists-notests.txt
mv deps/libzmq/CMakeLists.txt deps/libzmq/CMakeLists-tests.txt
mv deps/libzmq/CMakeLists-notests.txt deps/libzmq/CMakeLists.txt
mv deps/libzmq/src/compat.hpp deps/libzmq/src/compat.orig.hpp
echo "#define HAVE_STRNLEN" > deps/libzmq/src/compat.hpp
cat deps/libzmq/src/compat.orig.hpp >> deps/libzmq/src/compat.hpp
mv CMakeLists.txt CMakeLists.orig.txt
mv CMakeLists-centos9.txt CMakeLists.txt
mkdir build
cd build/
source ~/.bashrc
cmake -G Ninja ..
ninja
ninja install
#scl enable devtoolset-9 ./helper.sh
docker pull racelab/cspot-docker-centos7
docker tag racelab/cspot-docker-centos7 cspot-docker-centos7

if ! [[ $LD_LIBRARY_PATH == *"/usr/local/lib"* ]]; then
    echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> ~/.bashrc
    source ~/.bashrc
fi
cp ../SELF-TEST.sh ./bin
cd ./bin
./SELF-TEST.sh