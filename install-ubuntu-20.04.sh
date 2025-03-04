#!/bin/bash



apt-get update && apt-get -y install ninja-build g++-9 cmake g++
apt-get remove docker docker-engine docker.io containerd runc
apt -y  install docker.io
apt-get -y install libssl-dev libyaml-dev
cd cspot/
git submodule update --init --recursive
mkdir build
cd build/
cmake -G Ninja ..
ninja
ninja install
cp ../SELF-TEST.sh ./bin
docker pull racelab/cspot-docker-centos7
docker tag racelab/cspot-docker-centos7 cspot-docker-centos7

if ! [[ $LD_LIBRARY_PATH == *"/usr/local/lib"* ]]; then
    echo -e "if ! [[ \$LD_LIBRARY_PATH == *\"/usr/local/lib\"* ]]; then\nexport  LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/usr/local/lib\"\nfi" >> ~/.bashrc
    source ~/.bashrc
fi
cd ./bin
./SELF-TEST.sh
