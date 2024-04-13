#!/bin/bash


apt-get update && apt-get -y install ninja-build cmake g++
apt-get remove -y docker docker-engine docker.io containerd runc
apt -y  install docker.io
git submodule update --init --recursive
mkdir -p build
cd build/
cmake -G Ninja ..
ninja
ninja install
docker pull racelab/cspot-docker-centos7:armv6
docker tag racelab/cspot-docker-centos7:armv6 cspot-docker-centos7
cp ../SELF-TEST.sh ./bin

if ! [[ $LD_LIBRARY_PATH == *"/usr/local/lib"* ]]; then
    echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> ~/.bashrc
    source ~/.bashrc
fi
cd ./bin
./SELF-TEST.sh
